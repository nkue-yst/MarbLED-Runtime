//
// Created by UraChihiro on 4/20/23.
//

#include <vector>
#include <string>
#include <thread>
#include <termios.h>
#include <future>
#include <queue>
#include <iostream>
#include <cmath>
#include <map>

#if !defined(NO_BOARD)
#include <libudev.h>
#endif

#include "serial.h"
#include "zmq.hpp"
#include "zmq_addon.hpp"

#define     BUFFER_MAX       10

typedef std::vector<std::vector<uint16_t>> f_img;

std::queue<f_img> sensor_queue;
bool exit_flg = false;

struct board{
    const char *serial;
    uint8_t version;
    uint8_t chain;
    uint8_t sensors;
    uint8_t modes;
};

struct container{
    char serial[256];
    uint16_t id;
    uint16_t controller_id;
    uint8_t version;
    uint8_t chain_num;
    uint8_t modes;
    int32_t layout_x;
    int32_t layout_y;
};

#if !defined(NO_BOARD)
const char *get_serial_num(const char *name){
    udev* ud = udev_new();
    udev_device* udv = udev_device_new_from_subsystem_sysname(ud, "tty", name);
    return udev_device_get_property_value(udv, "ID_SERIAL");
}
#endif

int get_board_ids(zmq::context_t *ctx, const char *conn_addr, board *brd, std::map<unsigned int, unsigned int> *ids){
    printf("Getting ID from storage.\n");

    // prepare publisher
    zmq::socket_t req(*ctx, zmq::socket_type::req);
    req.connect(conn_addr);

    printf("connected.\n");

    std::vector<zmq::message_t> recv_msgs;

    for(int i = 0; i < brd->chain; i++){
        recv_msgs.clear();

        container rqc{};
        strcpy(rqc.serial, brd->serial);
        rqc.chain_num = i;
        rqc.version = brd->version;
        rqc.modes = brd->modes;

        req.send(zmq::buffer("STORAGE REQ_BRDIDS"), zmq::send_flags::sndmore);
        req.send(zmq::buffer(&rqc, sizeof(rqc)), zmq::send_flags::none);

        zmq::recv_multipart(req, std::back_inserter(recv_msgs));
        if(recv_msgs.empty()) {
            std::cout << "timeout" << std::endl;
            continue;
        }
        if(recv_msgs.size() != 2) continue;

        recv_msgs.erase(recv_msgs.begin());

        container rc = *recv_msgs.at(0).data<container>();
        ids->insert(std::make_pair(i, rc.id));

        printf("CHAIN NUM[%d] = ID %d\n", i, rc.id);

    }

    req.close();
    return 0;
}


// BRD_DATA [Serial-Number] [chain] [chain-num] [Mode] [Sensor-Data...]
void publish_data(zmq::context_t *ctx, board *brd, const char *bind_addr, const std::map<unsigned int, unsigned int> *ids){
    printf("launch publisher\n");


    // prepare publisher
    zmq::socket_t publisher(*ctx, zmq::socket_type::pub);
    publisher.connect(bind_addr);

    printf("pub bind\n");


    char head[128];                                  // for data header
    f_img frame;                                    // get from fifo buffer


    while(!exit_flg){

        // retry when fifo is empty
        if(sensor_queue.empty()){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // get from fifo buffer
        frame = sensor_queue.front();
        sensor_queue.pop();

        // publish via zmq socket
        for(int i = 0; i < frame.size(); i++){

            int chain_num = (int)std::floor(i / brd->modes);

            std::snprintf(head, 128, "BRD_DATA %d %d %d %d",
                          ids->at(chain_num),
                          brd->chain,
                          chain_num,
                          i % brd->modes
                          );

            publisher.send(zmq::buffer(head), zmq::send_flags::sndmore);
            publisher.send(zmq::buffer(frame.at(i)), zmq::send_flags::none);

        }
    }
}

void store_buffer(const f_img& frm){
    // block when fifo buffer is max
    while(sensor_queue.size() >= BUFFER_MAX){
        std::cerr << "buffer is max" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    sensor_queue.push(frm);
}

void receive_data(serial *ser, board *brd){
    std::vector<tm_packet> pacs = std::vector<tm_packet>();
    f_img frame(brd->modes * brd->chain, std::vector<uint16_t>(brd->sensors));

    bool ret;

    while(!exit_flg) {
        ret = ser->read(&pacs);
        if(ret == 0)continue;

        for(tm_packet pac : pacs){

            // detect error
            if(pac.s_num >= brd->sensors || pac.mode >= brd->modes) continue;

            frame.at(pac.mode).at(pac.s_num) = pac.value;

            if(pac.s_num == brd->sensors -1 && pac.mode == brd->modes -1) store_buffer(frame);
        }
    }

}

void push_dummy(board *brd){
    std::vector<tm_packet> pacs = std::vector<tm_packet>();
    f_img frame(brd->modes * brd->chain, std::vector<uint16_t>(brd->sensors));
    for(auto &f : frame){
        std::fill(f.begin(), f.end(), 0xffff);
    }

    while(!exit_flg) {

        store_buffer(frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    }
}

void get_board_info(serial *ser, board *brd){
    std::vector<tm_packet> pacs = std::vector<tm_packet>();
    tm_packet info_pac = {};
    bool ret;
    bool endflg = false;

    while(!endflg){
        ret = ser->read(&pacs);
        if(!ret) continue;

        for(tm_packet pac: pacs){
            if(pac.s_num == 0xff){
                info_pac = pac;
                endflg = true;
                break;
            }
        }
    }

    brd->version = info_pac.mode;
    brd->chain = (info_pac.value >> 8) & 0xff;
    brd->sensors = info_pac.value & 0xff;
}

void launch(zmq::context_t *ctx, serial *ser, board *brd, const char *bind_addr, const std::map<unsigned int, unsigned int> *ids){

    std::future<void> pub_data_th = std::async(std::launch::async, publish_data, ctx, brd, bind_addr, ids);
#if !defined(NO_BOARD)
    std::future<void> pico_th = std::async(std::launch::async, receive_data, ser, brd);
#else
    std::future<void> pico_th = std::async(std::launch::async, push_dummy, brd);
#endif

    pub_data_th.wait();
    pico_th.wait();
}

void run(const char* port, const char* bind_addr, const char* info_addr, int modes){

    // print options
    printf("TM SERIAL PORT      : %s\n", port);
    printf("SENS CONNECT ADDR   : %s\n", bind_addr);
    printf("META CONNECT ADDR   : %s\n", info_addr);
    printf("SENSING MODES  : %d\n", modes);

    // init vars
    serial ser(port, B115200);
    board brd = {};
    char name[256];
    sscanf(port, "/dev/%s", name);

#if !defined(NO_BOARD)
    // open serial port
    int ret = ser.tm_open();
    if(ret != 0)exit(-1);

    printf("Waiting for board data...\n");

    // wait for board-info from serial-connection
    get_board_info(&ser, &brd);
    brd.modes = modes;
    brd.serial = get_serial_num(name);
#else
    printf("Test Mode Enabled.\n");
    brd.version = 4;
    brd.chain = 2;
    brd.modes = modes;
    brd.sensors = 18;
    brd.serial = "board_is_not_connected";
#endif

    printf("SERIAL_NUM  : %s\n", brd.serial);
    printf("BOARD_VER   : %d\n", brd.version);
    printf("BOARD_CHAIN : %d\n", brd.chain);
    printf("SENSORS     : %d\n", brd.sensors);

    zmq::context_t ctx(1);

    // get board ids
    std::map<unsigned int, unsigned int> ids{};
    get_board_ids(&ctx, info_addr, &brd, &ids);

    launch(&ctx, &ser, &brd, bind_addr, &ids);

    ser.close();
}

int main(int argc, char* argv[]){

    int c;
    const char* optstring = "t:p:b:i:";

    // enable error log from getopt
    opterr = 0;

    // sensing modes
    int s_modes = 5;
    // serial port
    char* ser_p = nullptr;
    // bind address
    char* bind_addr = nullptr;
    // connect address for publishing meta-data
    char* info_addr = nullptr;

    // get options
    while((c = getopt(argc, argv, optstring)) != -1){
        if(c == 'p') {
            ser_p = optarg;
        }else if(c == 'b') {
            bind_addr = optarg;
        }else if(c == 'i') {
            info_addr = optarg;
        }else{
            // parse error
            printf("unknown argument error\n");
            return -1;
        }
    }

    // check for serial port
    if(ser_p == nullptr){
        printf("require serial port option -p");
        return -1;
    }
    // check for bind address
    if(bind_addr == nullptr){
        printf("require bind address option -b");
        return -2;
    }
    // check for bind address
    if(info_addr == nullptr){
        printf("require storage address option -i");
        return -3;
    }

    run(ser_p, bind_addr, info_addr, s_modes);

    return 0;
}
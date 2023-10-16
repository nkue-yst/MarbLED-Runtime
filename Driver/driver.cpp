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
#include "board.h"
#include "utility.h"
#include "zmq.hpp"
#include "zmq_addon.hpp"

typedef std::vector<std::vector<uint16_t>> f_img;

std::queue<f_img> sensor_queue;
bool exit_flg = false;

struct controller{
    const char *serial;
    uint8_t version;
    uint8_t chain;
    uint8_t sensors;
    uint8_t modes;
};


#if !defined(NO_BOARD)
const char *get_serial_num(const char *name){
    udev* ud = udev_new();
    udev_device* udv = udev_device_new_from_subsystem_sysname(ud, "tty", name);
    return udev_device_get_property_value(udv, "ID_SERIAL");
}
#endif


/**
 * 基板IDをStorageノードから取得する
 * @param ctx           zmq context
 * @param conn_addr     Storage ノードのアドレス (e.g. tcp://127.0.0.1:8000)
 * @param brd           基板情報
 * @param ids           idリスト
 * @return
 */
void get_board_ids(const char *conn_addr, const char *serial, unsigned int chain, std::vector<unsigned int> *ids){

    for(int i = 0; i < chain; i++){
        ids->push_back( get_board_id(conn_addr, serial, i) );
    }

}


// BRD_DATA [Serial-Number] [chain] [chain-num] [Mode] [Sensor-Data...]
void publish_data(controller *brd, const char *bind_addr, const std::map<unsigned int, unsigned int> *ids){
    printf("launch publisher\n");


    // prepare publisher
    zmq::context_t ctx(1);
    zmq::socket_t publisher(ctx, zmq::socket_type::pub);
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



void receive_data(serial *ser, controller *brd){
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

            if(pac.s_num == brd->sensors -1 && pac.mode == brd->modes -1) store_buffer(frame, sensor_queue);
        }
    }

}

void push_dummy(controller *brd){
    std::vector<tm_packet> pacs = std::vector<tm_packet>();
    f_img frame(brd->modes * brd->chain, std::vector<uint16_t>(brd->sensors));
    for(auto &f : frame){
        std::fill(f.begin(), f.end(), 0xffff);
    }

    while(!exit_flg) {

        store_buffer(frame, sensor_queue);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    }
}

void subscribe_color(controller *brd, const char *addr, unsigned int bid, std::queue<f_color> fc){

    // init subscriber
    zmq::context_t ctx(2);
    zmq::socket_t subscriber(ctx, zmq::socket_type::sub);
    subscriber.connect(addr);

    // filter message
    char filter_string[256] = {};
    snprintf(filter_string, 256, "BRD_COLOR %d", bid);

    subscriber.set(zmq::sockopt::subscribe, filter_string);
    std::vector<zmq::message_t> recv_msgs;

    std::cout << "bind on " << addr << std::endl;

    while(true){
        recv_msgs.clear();

        zmq::recv_multipart(subscriber, std::back_inserter(recv_msgs));
        if(recv_msgs.empty()) {
            std::cout << "timeout" << std::endl;
            continue;
        }
        if(recv_msgs.size() != 4) continue;

        int bid, chain, c_num, mode;
        int ret = sscanf(recv_msgs.at(0).data<char>(), "BRD_DATA %d %d %d %d",
                         &bid,
                         &chain,
                         &c_num,
                         &mode);
        if(ret == EOF) continue; //fail to decode

        unsigned long d_size = recv_msgs.at(1).size() / sizeof(uint16_t);
        update_frames(bid, mode, recv_msgs.at(1).data<uint16_t>(), d_size, frames);
    }
}

void get_board_info(serial *ser, controller *brd){
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

void launch(Bucket *ser, const char *bind_addr, const std::vector<Board> *brds){

    std::future<void> pub_data_th = std::async(std::launch::async, publish_data, brd, bind_addr, ids);
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
    controller c = {};
    char name[256];
    sscanf(port, "/dev/%s", name);

#if !defined(NO_BOARD)
    // open serial port
    int ret = ser.tm_open();
    if(ret != 0)exit(-1);

    printf("Waiting for board data...\n");

    // wait for board-info from serial-connection
    get_board_info(&ser, &c);
    c.modes = modes;
    c.serial = get_serial_num(name);
#else
    printf("Test Mode Enabled.\n");
    c.version = 4;
    c.chain = 2;
    c.modes = modes;
    c.sensors = 18;
    c.serial = "board_is_not_connected";
#endif

    printf("SERIAL_NUM       : %s\n", c.serial);
    printf("CONTROLLER_VER   : %d\n", c.version);
    printf("BOARD_CHAIN      : %d\n", c.chain);
    printf("SENSORS          : %d\n", c.sensors);

    // get board ids
    std::vector<unsigned int> ids{};
    get_board_ids(info_addr, c.serial,  c.chain, &ids);

    // create board instance
    std::vector<Board> brds;
    for(unsigned int id : ids){
        brds.emplace_back(id, c.sensors, c.modes);
    }

    // launch
    launch(ser, bind_addr,  &ids);

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
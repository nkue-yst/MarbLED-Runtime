//
// Created by UraChihiro on 4/20/23.
//

#include <vector>
#include <string>
#include <thread>
#include <termios.h>
#include <future>
#include <queue>

#include <libudev.h>

#include "serial.h"
#include "zmq.hpp"

#define     BUFFER_MAX       10
#define     INFO_PUB         10

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

const char *get_serial_num(const char *name){
    udev* ud = udev_new();
    udev_device* udv = udev_device_new_from_subsystem_sysname(ud, "tty", name);
    return udev_device_get_property_value(udv, "ID_SERIAL");
}


// BRD_INFO [Serial-Number] [Board-Version] [Chain] [Sensors] [Modes]
zmq::message_t build_info_msg(board *brd){
    zmq::message_t brd_info(128);

    snprintf((char *)brd_info.data(), 128, "BRD_INFO %s %d %d %d %d",
             brd->serial,
             brd->version,
             brd->chain,
             brd->sensors,
             brd->modes);

    return brd_info;
}


// BRD_DATA [Serial-Number] [Mode] [Sensor-Data...]
void publish_data(board *brd, const char *bind_addr){
    printf("launch publisher\n");


    // prepare publisher
    zmq::context_t ctx(1);
    zmq::socket_t publisher(ctx, zmq::socket_type::pub);
    publisher.connect(bind_addr);

    printf("pub bind\n");


    char head[128];                                  // for data header
    f_img frame;                                    // get from fifo buffer
    uint32_t info_pub_tim = INFO_PUB;               // timing for publishing meta-data
    zmq::message_t info = build_info_msg(brd);      // meta-data


    while(!exit_flg){

        // retry when fifo is empty
        if(sensor_queue.empty()){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        printf("get from buffer\n");

        // get from fifo buffer
        frame = sensor_queue.front();
        sensor_queue.pop();

        // publish via zmq socket
        for(int i = 0; i < frame.size(); i++){

            std::snprintf(head, 128, "BRD_DATA %s %d", brd->serial, i);

            publisher.send(zmq::buffer(head), zmq::send_flags::sndmore);
            publisher.send(zmq::buffer(frame.at(i)), zmq::send_flags::none);

        }
        printf("published\n");

        // publish meta-data when info_pub_tim matched info_pub
        info_pub_tim++;
        if(info_pub_tim > INFO_PUB){
            publisher.send(info, zmq::send_flags::none);
            printf("pub info\n");
            info_pub_tim = 0;
        }
    }
}

void store_buffer(const f_img& frm){
    // block when fifo buffer is max
    while(sensor_queue.size() >= BUFFER_MAX){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    sensor_queue.push(frm);
    printf("store\n");
}

void receive_data(serial *ser, board *brd){
    std::vector<tm_packet> pacs = std::vector<tm_packet>();
    f_img frame(brd->modes, std::vector<uint16_t>(brd->sensors));

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

void launch(serial *ser, board *brd, const char *bind_addr){

    std::future<void> pub_data_th = std::async(std::launch::async, publish_data, brd, bind_addr);
    std::future<void> pico_th = std::async(std::launch::async, receive_data, ser, brd);

    pub_data_th.wait();
    pico_th.wait();
}

void run(const char* port, const char* bind_addr, int modes){

    // print options
    printf("TM SERIAL PORT : %s\n", port);
    printf("ZMQ BIND ADDR  : %s\n", bind_addr);
    printf("SENSING MODES  : %d\n", modes);

    // init vars
    serial ser(port, B115200);
    board brd = {};
    char name[256];
    sscanf(port, "/dev/%s", name);

    // open serial port
    int ret = ser.tm_open();
    if(ret != 0)exit(-1);

    printf("Waiting for board data...\n");

    // wait for board-info from serial-connection
    get_board_info(&ser, &brd);
    brd.modes = modes;
    brd.serial = get_serial_num(name);

    printf("Received the board data\n");
    printf("SERIAL_NUM  : %s\n", brd.serial);
    printf("BOARD_VER   : %d\n", brd.version);
    printf("BOARD_CHAIN : %d\n", brd.chain);
    printf("SENSORS     : %d\n", brd.sensors);

    launch(&ser, &brd, bind_addr);

    ser.close();
}

int main(int argc, char* argv[]){

    int c;
    const char* optstring = "t:p:b:";

    // enable error log from getopt
    opterr = 0;

    // sensing modes
    int s_modes = 5;
    // serial port
    char* ser_p = nullptr;
    // bind address
    char* bind_addr = nullptr;

    // get options
    while((c = getopt(argc, argv, optstring)) != -1){
        if(c == 't'){
            s_modes = (int)strtol(optarg, nullptr,  10);
        }else if(c == 'p') {
            ser_p = optarg;
        }else if(c == 'b'){
            bind_addr = optarg;
        }else{
            // parse error
            printf("unknown argument error\n");
            return -1;
        }
    }

    // check for serial port
    if(ser_p == nullptr){
        printf("require serial port option");
        return -1;
    }
    // check for bind address
    if(bind_addr == nullptr){
        printf("require bind address option");
        return -2;
    }

    run(ser_p, bind_addr, s_modes);

    return 0;
}
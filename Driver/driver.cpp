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


// BRD_DATA [Board-Id] [Mode] [Sensor-Data...]
void publish_data(const char *bind_addr, std::vector<Board> *brds){
    printf("launch publisher\n");


    // prepare publisher
    zmq::context_t ctx(1);
    zmq::socket_t publisher(ctx, zmq::socket_type::pub);
    publisher.connect(bind_addr);

    printf("pub bind\n");


    char head[128];                                  // for data header
    f_img frame;                                    // get from fifo buffer


    while(!exit_flg){

        for(auto &brd : *brds){
            int ret = brd.pop_sensor_values(&frame);
            if(ret != 0) continue;

            // publish via zmq socket
            for(int i = 0; i < frame.size(); i++){

                std::snprintf(head, 128, "BRD_DATA %d %d",
                              brd.get_id(),
                              i
                );

                publisher.send(zmq::buffer(head), zmq::send_flags::sndmore);
                publisher.send(zmq::buffer(frame.at(i)), zmq::send_flags::none);

            }

        }

    }
}



void receive_data(Bucket *ser, std::vector<Board> *brds){
    std::vector<tm_packet> pacs = std::vector<tm_packet>();

    bool ret;
    int data_per_board = brds->at(0).get_sensors() * brds->at(0).get_modes();
    int sensors = brds->at(0).get_sensors();
    int modes = brds->at(0).get_modes();

    f_img frame(brds->at(0).get_modes(), std::vector<uint16_t>(brds->at(0).get_sensors()));

    while(!exit_flg) {
        ret = ser->read(&pacs);
        if(ret == 0)continue;

        for(tm_packet pac : pacs){
            int brd = floor(pac.d_num / data_per_board);
            int sensor = pac.d_num % sensors;
            int mode = pac.d_num % modes;

            frame.at(mode).at(sensor) = pac.value;
            if(mode == modes -1 && sensor == sensors - 1) brds->at(brd).store_sensor(&frame);
        }
    }

}

void push_dummy(std::vector<Board> *brds){

    while(!exit_flg) {
        for(auto &b : *brds){

            f_img frame(b.get_modes(), std::vector<uint16_t>(b.get_sensors()));
            for(auto &f : frame){
                std::fill(f.begin(), f.end(), 0xffff);
            }
            b.store_sensor(&frame);

        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

    }
}

void write_color(Bucket *ser, std::vector<Board> *brds){
    uint8_t buf[2048] = {};
    uint8_t r[18][18] = {};
    uint8_t g[18][18] = {};
    uint8_t b[18][18] = {};
    unsigned int ignore_pix[36] = {
            19, 22, 25, 28, 31, 34,
            73, 76, 79, 82, 85, 88,
            127, 130, 133, 136, 139, 142,
            181, 184, 187, 190, 193, 196,
            235, 238, 241, 243, 246, 249,
            289, 292, 295,298, 301, 304,
    };

    while(true) {
        int cnt = 0;
        for (auto &brd: *brds) {

            f_color c(3, std::vector<uint8_t>(18 * 18));

            int ret = brd.pop_color_values(&c);
            if(ret < 0){
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            if (c.size() != 3) continue;

            // Reverse odd rows
            for (int i = 0; i < c.at(0).size(); i++) {

                int row = (int) floor(i / 18.0);
                int col = i % 18;
                int pol = row % 2;

                if (pol) col = 17 - col;
                r[row][col] = c.at(0).at(i);
                g[row][col] = c.at(1).at(i);
                b[row][col] = c.at(2).at(i);

            }

            // Ignore pixel values corresponding to sensor positions
            int pix_cnt = 0;
            for (int i = 0; i < 18; i++) {
                for (int j = 0; j < 18; j++) {

                    // check ignore pixels
                    bool ignore = false;
                    for (unsigned int p: ignore_pix) {
                        if (p == pix_cnt) {
                            ignore = true;
                            break;
                        }
                    }
                    pix_cnt++;

                    if (ignore) continue;

                    buf[cnt] = r[i][j];
                    cnt++;
                    buf[cnt] = g[i][j];
                    cnt++;
                    buf[cnt] = b[i][j];
                    cnt++;
                }
            }
        }
        if(cnt == 0)
        ser->transfer(buf, cnt);
    }
}

void subscribe_color(const char *addr, Board *brd){

    // init subscriber
    zmq::context_t ctx(2);
    zmq::socket_t subscriber(ctx, zmq::socket_type::sub);
    subscriber.connect(addr);

    // filter message
    char filter_string[256] = {};
    snprintf(filter_string, 256, "BRD_COLOR %d", brd->get_id());

    subscriber.set(zmq::sockopt::subscribe, filter_string);
    std::vector<zmq::message_t> recv_msgs;
    f_color fc(3, std::vector<uint8_t>(brd->get_sensors()));

    std::cout << "ID : " << brd->get_id() << " color is connected on " << addr << std::endl;

    while(true){
        recv_msgs.clear();

        zmq::recv_multipart(subscriber, std::back_inserter(recv_msgs));
        if(recv_msgs.empty()) {
            std::cout << "timeout" << std::endl;
            continue;
        }
        if(recv_msgs.size() != 4) continue;

        int bid;
        int ret = sscanf(recv_msgs.at(0).data<char>(), "BRD_COLOR %d",
                         &bid);
        if(ret == EOF) continue; //fail to decode

        unsigned long d_size_r = recv_msgs.at(1).size() / sizeof(uint8_t);
        unsigned long d_size_g = recv_msgs.at(2).size() / sizeof(uint8_t);
        unsigned long d_size_b = recv_msgs.at(3).size() / sizeof(uint8_t);

        if(d_size_b != d_size_r || d_size_b != d_size_g){
            std::cerr << "invalid data size (color) bid=" << brd->get_id() << std::endl;
            continue;
        }

        f_color c(3, std::vector<uint8_t>(18 * 18));
        for(int i = 0; i < d_size_r; i++){
            c.at(0).at(i) = recv_msgs.at(1).data<uint8_t>()[i];
            c.at(1).at(i) = recv_msgs.at(2).data<uint8_t>()[i];
            c.at(2).at(i) = recv_msgs.at(3).data<uint8_t>()[i];
        }
        brd->store_color(&c);
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
            if(((pac.d_num >> 8) & 0xff) == 0xff){
                info_pac = pac;
                endflg = true;
                break;
            }
        }
    }

    brd->version = info_pac.d_num & 0xff;
    brd->chain = (info_pac.value >> 8) & 0xff;
    brd->sensors = info_pac.value & 0xff;
}

void launch(Bucket *ser, const char *bind_addr, const char *color_addr, std::vector<Board> *brds){

    // data publish
    std::future<void> pub_data_th = std::async(std::launch::async, publish_data, bind_addr, brds);
#if !defined(NO_BOARD)
    std::future<void> pico_th = std::async(std::launch::async, receive_data, ser, brds);
    std::future<void> write_th = std::async(std::launch::async, write_color, ser, brds);
    std::vector<std::future<void>> sub_color_ths;
    for(auto &brd: *brds){
        sub_color_ths.emplace_back(std::async(std::launch::async, subscribe_color, color_addr, &brd));
    }
#else
    // dummy data store to queue
    std::future<void> pico_th = std::async(std::launch::async, push_dummy, brd);
#endif

    pub_data_th.wait();
    pico_th.wait();
    write_th.wait();
    for(auto &sct : sub_color_ths) {
        sct.wait();
    }
}

void run(const char* port, const char* bind_addr, const char* info_addr, const char* color_addr){
    int modes = 5;

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
    launch(&ser, bind_addr, color_addr, &brds);

    ser.close();

}

int main(int argc, char* argv[]){

    int c;
    const char* optstring = "t:p:b:i:c:";

    // enable error log from getopt
    opterr = 0;

    // serial port
    char* ser_p = nullptr;
    // bind address
    char* bind_addr = nullptr;
    // connect address for publishing meta-data
    char* info_addr = nullptr;
    // connect address for subscribe color-data
    char* color_addr = nullptr;

    // get options
    while((c = getopt(argc, argv, optstring)) != -1){
        if(c == 'p') {
            ser_p = optarg;
        }else if(c == 'b') {
            bind_addr = optarg;
        }else if(c == 'i') {
            info_addr = optarg;
        }else if(c == 'c'){
            color_addr = optarg;
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
    // check for bind address
    if(color_addr == nullptr){
        printf("require color address option -c");
        return -3;
    }

    run(ser_p, bind_addr, info_addr, color_addr);

    return 0;
}
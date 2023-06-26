//
// Created by ura on 6/26/23.
//


#include <iostream>
#include <termios.h>

#include <opencv2/opencv.hpp>
#include <future>
#include <thread>

#include "frame.h"

#include "zmq.hpp"
#include "zmq_addon.hpp"


#define BUFFER_MAX 20


struct board{
    char serial[256];
    uint8_t version;
    uint8_t chain;
    uint8_t sensors;
    uint8_t modes;
};

typedef std::vector<std::vector<uint16_t>> f_img;
std::queue<f_img> receive_buffer;


void store_buffer(const char *serial, int mode, const uint16_t *data){
    // block when fifo buffer is max
    while(receive_buffer.size() >= BUFFER_MAX){
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    receive_buffer.push(frm);
    printf("store\n");
}

void zmq_receive(const char *addr){
    zmq::context_t ctx(1);
    zmq::socket_t subscriber(ctx, zmq::socket_type::sub);
    subscriber.bind(addr);

    subscriber.set(zmq::sockopt::subscribe, "BRD_DATA");
    //subscriber.set(zmq::sockopt::rcvtimeo, 500);
    std::vector<zmq::message_t> recv_msgs;

    while(1){
        recv_msgs.clear();

        char brd_serial[256];
        int mode;

        zmq::recv_multipart(subscriber, std::back_inserter(recv_msgs));
        if(recv_msgs.empty()) {
            std::cout << "timeout" << std::endl;
            continue;
        }

        int ret = sscanf(recv_msgs.at(0).data<char>(), "BRD_DATA %s %d ", brd_serial, &mode);
        if(ret == EOF) continue; //fail to decode

        store_buffer(brd_serial, mode, recv_msgs.at(1).data<uint16_t>());

        auto tm = recv_msgs.at(1).data<uint16_t>();
        tmp[mode].assign(tm, tm + recv_msgs.at(1).size());

        if(mode == brd->modes - 1) store_buffer(tmp);

    }
}

void run(const char *addr, const char *storage_addr){
    std::future<void> recv_meta = std::async(std::launch::async, zmq_receive, addr);
    recv_meta.wait();
}

int main(int argc, char* argv[]){

    int c;
    const char* optstring = "d:b:";

    // enable error log from getopt
    opterr = 0;

    // bind address
    char* bind_addr = nullptr;
    char* storage_addr = nullptr;

    // get options
    while((c = getopt(argc, argv, optstring)) != -1){
        if(c == 'd') {
            storage_addr = optarg;
        }else if(c == 'b'){
            bind_addr = optarg;
        }else{
            // parse error
            printf("unknown argument error\n");
            return -1;
        }
    }

    // check for serial port
    if(storage_addr == nullptr){
        printf("require storage address");
        return -1;
    }
    // check for bind address
    if(bind_addr == nullptr){
        printf("require bind address option");
        return -2;
    }

    run(bind_addr, storage_addr);

    return 0;
}

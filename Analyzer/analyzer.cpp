//
// Created by ura on 6/26/23.
//


#include <iostream>
#include <termios.h>

#include <opencv2/opencv.hpp>
#include <future>
#include <thread>

#include "frame.h"


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
std::map<const char*, frame> ser_table;





void run(const char *addr, const char *storage_addr){
    std::future<void> recv_meta = std::async(std::launch::async, frame::zmq_receive, addr);
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

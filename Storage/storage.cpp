//
// Created by ura on 6/21/23.
//
#include <string>
#include <unistd.h>
#include <iostream>

#include <zmq.hpp>
#include <zmq_addon.hpp>

#include <sqlite3.h>
#include <future>

#include "db.h"

std::vector<std::string> serial_cache;


void receive_meta(db *db_s, const char* addr){

    zmq::context_t ctx(1);
    zmq::socket_t subscriber(ctx, zmq::socket_type::sub);
    subscriber.bind(addr);
    std::cout <<  "bind addr" << std::endl;

    subscriber.set(zmq::sockopt::subscribe, "BRD_INFO");
    std::vector<zmq::message_t> recv_msgs;


    while(1){
        recv_msgs.clear();

        board brd = {};

        zmq::recv_multipart(subscriber, std::back_inserter(recv_msgs));
        if(recv_msgs.empty()) {
            std::cout << "timeout" << std::endl;
            continue;
        }

        int ret = sscanf(recv_msgs.at(0).data<char>(),
                "BRD_INFO %s %d %d %d %d",
                brd.serial,
                &brd.version,
                &brd.chain,
                &brd.sensors,
                &brd.modes);

        if(ret == EOF) continue; // fail to decode

        auto itr = std::find(serial_cache.begin(), serial_cache.end(),brd.serial);
        if(itr == serial_cache.end()){
            db_s->add_board(&brd);
            serial_cache.emplace_back(brd.serial);
        }

    }
}

void req_thread(db *db_s, const char *addr){
    zmq::context_t ctx(1);
    zmq::socket_t subscriber(ctx, zmq::socket_type::rep);
    subscriber.bind(addr);
    std::cout <<  "bind addr" << std::endl;

    std::vector<zmq::message_t> recv_msgs;
    while(1){
        recv_msgs.clear();

        zmq::recv_multipart(subscriber, std::back_inserter(recv_msgs));
        if(recv_msgs.empty()) {
            std::cout << "timeout" << std::endl;
            continue;
        }

        char command[256]{};
        int ret = sscanf(recv_msgs.at(0).data<char>(),
                         "STORAGE %s",
                         command);
        if(ret == EOF) continue; // fail to decode

        if(strcmp(command, "REQ_LAYOUT") == 0) {

        }else if(strcmp(command, "STR_LAYOUT") == 0){

        }


    }
}


void run(const char *db_path, const char *addr, const char *req_addr){
    db db_s(db_path);
    int ret = db_s.init();
    if(ret < 0)return;

    std::future<void> recv_meta = std::async(std::launch::async, receive_meta, &db_s, addr);
    std::future<void> recv_req = std::async(std::launch::async, req_thread, &db_s, req_addr);
    recv_meta.wait();
    recv_req.wait();
}

int main(int argc, char* argv[]){

    int c;
    const char* optstring = "d:b:r:";

    // enable error log from getopt
    opterr = 0;

    // sqlite db_path
    char *db_path = nullptr;

    // bind address
    char* bind_addr = nullptr;
    char* req_addr = nullptr;

    // get options
    while((c = getopt(argc, argv, optstring)) != -1){
        if(c == 'd') {
            db_path = optarg;
        }else if(c == 'b'){
            bind_addr = optarg;
        }else if(c == 'r'){
            req_addr = optarg;
        }else{
            // parse error
            printf("unknown argument error\n");
            return -1;
        }
    }

    // check for serial port
    if(db_path == nullptr){
        printf("require data-base path");
        return -1;
    }
    // check for bind address
    if(bind_addr == nullptr){
        printf("require bind address option");
        return -2;
    }
    // check for bind address
    if(req_addr == nullptr){
        printf("require request bind address option");
        return -2;
    }
    run(db_path, bind_addr, req_addr);

    return 0;
}

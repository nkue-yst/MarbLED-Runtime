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

void get_layout(unsigned int id, board *brd){

}

void store_layout(unsigned int id, int x, int y){

}

unsigned int acq_board_id(db *db_s, board *brd){
    int count = db_s->is_controller_exist(brd->serial);
    if(count == 0){
        db_s->add_controller(brd->serial, 0, 1);
    }
    unsigned int cid = db_s->get_controller_id(brd->serial);

    count = db_s->is_board_exist(cid, brd->chain_num);
    if(count == 0){
        db_s->add_board(brd);
    }

    return db_s->get_board_id(cid, brd->chain_num);

}

void reply(zmq::socket_t *soc, std::vector<board> brd){
    soc->send(zmq::buffer("STORAGE REPLY"), zmq::send_flags::sndmore);
    if(brd.size() == 1){
        soc->send(zmq::buffer(&brd.at(0), sizeof(brd.at(0))), zmq::send_flags::none);
        return;
    }

    zmq::send_flags flags = zmq::send_flags::sndmore;
    for(int i = 0; i < brd.size(); i++){
        if(i == brd.size() -1) flags = zmq::send_flags::none;
        soc->send(zmq::buffer(&brd.at(i), sizeof(brd.at(i))), flags);
    }

}

void req_thread(db *db_s, const char *addr){
    zmq::context_t ctx(1);
    zmq::socket_t subscriber(ctx, zmq::socket_type::rep);
    subscriber.bind(addr);
    std::cout <<  "bind addr" << std::endl;

    std::vector<zmq::message_t> recv_msgs;
    std::vector<unsigned int> board_cache;

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

        // exec commands
        if(strcmp(command, "REQ_LAYOUT") == 0) {

        }else if(strcmp(command, "STR_LAYOUT") == 0){

        }else if(strcmp(command, "REQ_BRDIDS") == 0){

            board brd{};
            int ret = sscanf(recv_msgs.at(1).data<char>(),
                             "%s %d %d %d",
                             brd.serial,
                             &brd.chain_num,
                             &brd.version,
                             &brd.modes);
            if(ret == EOF) continue; // fail to decode

            brd.id = acq_board_id(db_s, &brd);
            board_cache.emplace_back(brd.id);

            reply(&subscriber, std::vector<board>{brd});

        }


    }
}


void run(const char *db_path, const char *addr, const char *req_addr){
    db db_s(db_path);
    int ret = db_s.init();
    if(ret < 0)return;

    std::future<void> recv_req = std::async(std::launch::async, req_thread, &db_s, req_addr);
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

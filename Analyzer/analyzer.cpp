//
// Created by ura on 6/26/23.
//


#include <iostream>

#include <opencv2/opencv.hpp>
#include <future>
#include <thread>
#include <queue>

#include "frame.h"
#include "mapper.h"


void get_connected_boards(const char *addr, std::vector<board> *brds) {
    zmq::context_t ctx(1);
    zmq::socket_t req(ctx, zmq::socket_type::req);
    req.connect(addr);

    // build a message requesting a connected board
    char request[128] = "STORAGE REQ_LAYOUT";

    // send request
    req.send(zmq::buffer(request), zmq::send_flags::none);

    // wait for replies
    std::vector<zmq::message_t> recv_msgs;
    zmq::recv_multipart(req, std::back_inserter(recv_msgs));

    // erase header ( STORAGE REPLY )
    if(!recv_msgs.empty()) recv_msgs.erase(recv_msgs.begin());

    req.close();

    for(auto & recv_msg : recv_msgs) {
        board c = *recv_msg.data<board>();
        brds->push_back(c);
    }

}

void update_frames(const uint16_t bid, const uint16_t mode, const uint16_t *data, const unsigned long len, std::vector<frame> *frames){
    for(auto & frame : *frames){
        if(bid == frame.get_id()) frame.update(mode, data, len);
    }
}

void subscribe_data(std::vector<frame> *frames, const char *addr){
    std::cout << "subscribing" << std::endl;
    zmq::context_t ctx(2);
    zmq::socket_t subscriber(ctx, zmq::socket_type::sub);
    subscriber.bind(addr);

    subscriber.set(zmq::sockopt::subscribe, "BRD_DATA");
    std::vector<zmq::message_t> recv_msgs;

    std::cout << "bind on " << addr << std::endl;

    while(true){
        recv_msgs.clear();

        zmq::recv_multipart(subscriber, std::back_inserter(recv_msgs));
        if(recv_msgs.empty()) {
            std::cout << "timeout" << std::endl;
            continue;
        }
        if(recv_msgs.size() != 2) continue;

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


void update_sens_img(Mapper *me){
    while(true){
        me->update();
    }
}


void launch(std::vector<frame> *frms, const char *addr, Mapper *mapper){
    std::future<void> recv_meta = std::async(std::launch::async, subscribe_data, frms, addr);
    std::future<void> img_update = std::async(std::launch::async, update_sens_img, mapper);
    recv_meta.wait();
    img_update.wait();
}


void run(const char *addr, const char *storage_addr){

    std::cout << "Getting board data..." << std::endl;

    // get board layout from storage
    std::vector<board> brds;
    get_connected_boards(storage_addr, &brds);

    // check board count
    if(brds.empty()){
        std::cerr << "No modules are connected." << std::endl
        << "Connected one or more modules and reboot.";
        exit(-1);
    }

    // create frame instance by board data
    std::vector<frame> frms;
    std::cout << "Connected boards" << std::endl;
    for(auto brd : brds){
        std::cout << "BID : " << brd.id << std::endl;
        frms.emplace_back(brd);
    }

    Mapper mapper(&frms);

    launch(&frms, addr, &mapper);
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

//
// Created by ura on 6/26/23.
//


#include <iostream>

#include <opencv2/opencv.hpp>
#include <future>
#include <thread>

#include "frame.h"
#include "mapper.h"



std::vector<board> get_connected_boards(const char *addr) {
    std::vector<board> brds;

    zmq::context_t ctx(1);
    zmq::socket_t req(ctx, zmq::socket_type::req);
    req.connect(addr);

    // build a message requesting a connected board
    char request[128] = {};
    std::snprintf(request, 128, "REQ_STORAGE CONNECTED_BRDS");

    // send request
    req.send(zmq::buffer(request), zmq::send_flags::none);

    // wait for replies
    std::vector<zmq::message_t> recv_msgs;
    zmq::recv_multipart(req, std::back_inserter(recv_msgs));

    req.close();

    for(auto & recv_msg : recv_msgs){
        board brd = {};
        int ret = std::sscanf(recv_msg.data<char>(),
                              "CONNECTED_BRD %s %s %s %s %s %s",
                              brd.serial,
                              &brd.version,
                              &brd.chain,
                              &brd.num,
                              &brd.sensors,
                              &brd.modes);
        if(ret == EOF) continue;

        brds.push_back(brd);
    }

    return brds;

}

void subscribe_data(Mapper *map, const char *addr){
    zmq::context_t ctx(1);
    zmq::socket_t subscriber(ctx, zmq::socket_type::sub);
    subscriber.bind(addr);

    subscriber.set(zmq::sockopt::subscribe, "BRD_DATA");
    std::vector<zmq::message_t> recv_msgs;

    while(true){
        recv_msgs.clear();

        zmq::recv_multipart(subscriber, std::back_inserter(recv_msgs));
        if(recv_msgs.empty()) {
            std::cout << "timeout" << std::endl;
            continue;
        }

        char serial[256] = {};
        int chain, c_num, mode;
        int ret = sscanf(recv_msgs.at(0).data<char>(), "BRD_DATA %s %d %d %d",
                         serial,
                         &chain,
                         &c_num,
                         &mode);
        if(ret == EOF) continue; //fail to decode

        map->update(serial, c_num, mode, recv_msgs.at(1).data<uint16_t>());

    }
}


void analyze(Mapper *mapper){

}


void run(const char *addr, const char *storage_addr){
    Mapper mapper;

    std::cout << "getting board data" << std::endl;
    std::vector<board> brds = get_connected_boards(storage_addr);
    mapper.set_connected_boards(brds);
    std::cout << "ok" << std::endl;

    std::future<void> recv_meta = std::async(std::launch::async, subscribe_data, &mapper, addr);

    analyze(&mapper);

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

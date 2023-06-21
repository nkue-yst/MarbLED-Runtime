//
// Created by ura on 6/21/23.
//
#include <string>
#include <unistd.h>
#include <iostream>

#include <zmq.hpp>
#include <zmq_addon.hpp>

#include <sqlite3.h>

void exec_sql(const char *db_path, const char *sql){
    sqlite3 *db = nullptr;
    int ret = sqlite3_open(db_path, &db);
    if(ret != SQLITE_OK){
        std::cerr << "db open error" << std::endl;
        return;
    }



    sqlite3_close(db);
}


void run(const char *db, const char *addr){
    zmq::context_t ctx(1);
    zmq::socket_t subscriber(ctx, zmq::socket_type::sub);
    subscriber.bind(addr);

    subscriber.set(zmq::sockopt::subscribe, "BRD_INFO");
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
        std::cout << "out" << std::endl;

        //sscanf(recv_msgs.at(0).data<char>(), "BRD_DATA %s %d ", brd_serial, &mode);
        //std::cout << brd_serial << " : [" << mode << "] [" << recv_msgs.at(1).data<uint16_t>()[30] << "] "<< std::endl;

    }
}

int main(int argc, char* argv[]){

    int c;
    const char* optstring = "t:d:b:";

    // enable error log from getopt
    opterr = 0;

    // sensing modes
    int s_modes = 5;
    // serial port
    char* db_path = nullptr;
    // bind address
    char* bind_addr = nullptr;

    // get options
    while((c = getopt(argc, argv, optstring)) != -1){
        if(c == 't'){
            s_modes = (int)strtol(optarg, nullptr,  10);
        }else if(c == 'd') {
            db_path = optarg;
        }else if(c == 'b'){
            bind_addr = optarg;
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

    run(db_path, bind_addr);

    return 0;
}

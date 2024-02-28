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

#include "serial.h"
#include "eth.h"
#include "board.h"
#include "utility.h"
#include "zmq.hpp"
#include "zmq_addon.hpp"

// connection types
#define ETH_PORT_BASE   8002

typedef std::vector<std::vector<uint16_t>> f_img;

bool exit_flg = false;


/**
 * 基板IDをStorageノードから取得する
 * @param ctx           zmq context
 * @param conn_addr     Storage ノードのアドレス (e.g. tcp://127.0.0.1:8000)
 * @param brd           基板情報
 * @param ids           idリスト
 * @return
 */
void get_board_ids(const char *conn_addr, const char *serial, unsigned int chain, std::vector<unsigned int> *ids, unsigned int version, unsigned int modes){

    for(int i = 0; i < chain; i++){
        ids->push_back( get_board_id(conn_addr, serial, i, version, modes) );
    }

}


/**
 * センサーデータをpublish
 * @param bind_addr     Analyzerノードのアドレス
 * @param brds          Boardインスタンス群
 */
void publish_data(const char *bind_addr, std::vector<Board> *brds){
    printf("launch publisher\n");


    // prepare publisher
    zmq::context_t ctx(1);
    zmq::socket_t publisher(ctx, zmq::socket_type::pub);
    publisher.connect(bind_addr);

    printf("pub bind\n");


    char head[128];                                  // for data header


    while(!exit_flg){

        for(auto &brd : *brds){
            f_img frame(brd.get_modes(), std::vector<uint16_t>(brd.get_sensors()));

            int ret = brd.pop_sensor_values(&frame);
            if (ret < 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;  //
            }

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


/**
 * 基板からセンサーデータ取得
 * @param ser   シリアル通信インスタンス
 * @param brds  Boardインスタンス群
 */
void receive_data(Bucket *ser, std::vector<Board> *brds){

    int sr = ser->tm_open();
    if(sr < 0){
        std::cerr << "socket open error [data receive]" << std::endl;
        return;
    }

    std::vector<tm_packet> pacs = std::vector<tm_packet>();
    std::vector<f_img> frames;
    for(int i = 0; i < brds->size(); i++){
        frames.emplace_back(brds->at(0).get_modes(), std::vector<uint16_t>(brds->at(0).get_sensors()));
    }

    bool ret;
    int data_per_board = brds->at(0).get_sensors() * brds->at(0).get_modes();
    int sensors = brds->at(0).get_sensors();
    int modes = brds->at(0).get_modes();

    while(!exit_flg) {
        ret = ser->read(&pacs);
        if (ret == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;  // データなければリトライ
        }
        for(tm_packet pac : pacs){
            int brd = pac.d_num / data_per_board;
            int sensor = pac.d_num % sensors;
            int mode = pac.d_num % modes;

            frames.at(brd).at(mode).at(sensor) = pac.value;
        }

        for(int i = 0; i < brds->size(); i++){
            brds->at(i).store_sensor(&frames.at(i));
        }
    }

    ser->close();
}


/**
 * ダミーデータ生成（テスト用）
 * @param brds  Boardインスタンス群
 */
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

/**
 * 基板に色情報を送信
 * TODO: （基板に固有の処理すぎる）
 * @param ser   シリアル通信インスタンス
 * @param brds  Boardインスタンス群
 */
void write_color(const char *brd_addr, std::vector<Board> *brds){

    std::map<unsigned int, Eth> soc_vec;
    for(int i = 0; i < brds->size(); i++){
        soc_vec.insert(
                std::make_pair(
                        brds->at(i).get_id(),
                        Eth(brd_addr, ETH_PORT_BASE + i, ETH_CONN_UDP, brd_master[4].sensors * brd_master[4].modes)
                        )
                        );
        int sr = soc_vec.at(brds->at(i).get_id()).tm_open();
        if(sr < 0){
            std::cerr << "socket open error [color write]" << std::endl;
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

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

    while (!exit_flg) {

        for(auto &brd: *brds) {
            int cnt = 0;

            f_color c(3, std::vector<uint8_t>(18 * 18));    // TODO: この初期化をいい感じにしたい

            // Boardインスタンスから色データをpop
            int ret = brd.pop_color_values(&c);
            if (ret < 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;  // データなければリトライ
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

                    buf[cnt] = g[i][j];
                    cnt++;
                    buf[cnt] = r[i][j];
                    cnt++;
                    buf[cnt] = b[i][j];
                    cnt++;
                }
            }
            if (cnt == 0)continue;
            soc_vec.at(brd.get_id()).transfer(buf, cnt);
        }
    }

    for(const auto& soc : soc_vec){
        soc.second.close();
    }
}

/**
 * 色情報をSubscribe
 * @param addr  bind アドレス
 * @param brd   Board インスタンス
 */
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

        // 受信データサイズを計算
        unsigned long d_size_r = recv_msgs.at(1).size() / sizeof(uint8_t);
        unsigned long d_size_g = recv_msgs.at(2).size() / sizeof(uint8_t);
        unsigned long d_size_b = recv_msgs.at(3).size() / sizeof(uint8_t);

        // データサイズをチェック
        if(d_size_b != d_size_r || d_size_b != d_size_g){
            std::cerr << "invalid data size (color) bid=" << brd->get_id() << std::endl;
            continue;
        }

        // 色データをBoardインスタンスのqueueへstore （ここの処理どうにかしたい）
        f_color c(3, std::vector<uint8_t>(18 * 18));
        for(int i = 0; i < d_size_r; i++){
            c.at(0).at(i) = recv_msgs.at(1).data<uint8_t>()[i];
            c.at(1).at(i) = recv_msgs.at(2).data<uint8_t>()[i];
            c.at(2).at(i) = recv_msgs.at(3).data<uint8_t>()[i];
        }
        brd->store_color(&c);
    }
}


void launch(const char *brd_addr, const char *bind_addr, const char *color_addr, std::vector<Board> *brds){

    // センサーデータのPublishスレッド
    std::future<void> pub_data_th = std::async(std::launch::async, publish_data, bind_addr, brds);

    // 基板データ取得　スレッド
    Eth receive_soc(brd_addr, ETH_PORT_BASE + brds->size(), ETH_CONN_TCP, brd_master[4].sensors * brd_master[4].modes * (int)brds->size());
    std::future<void> pico_th = std::async(std::launch::async, receive_data, &receive_soc, brds);

    // 色データ送信　スレッド
    std::future<void> send_color_th = std::async(std::launch::async, write_color, brd_addr, brds);

    // 色データSubscribe スレッド群
    std::vector<std::future<void>> sub_color_ths;
    for(auto &brd: *brds){
        sub_color_ths.emplace_back(std::async(std::launch::async, subscribe_color, color_addr, &brd));
    }

    // スレッド待機
    pub_data_th.wait();
    pico_th.wait();
    send_color_th.wait();
    for(auto &sct : sub_color_ths) {
        sct.wait();
    }
}


/**
 * 初期化など
 * @param port          シリアルポート
 * @param bind_addr     Analyzerノードのアドレス
 * @param info_addr     Storageノードのアドレス
 * @param color_addr    色情報Bindアドレス
 */
void run(const char* port, const char* bind_addr, const char* info_addr, const char* color_addr, int version, int chain){

    // print options
    printf("TM SERIAL PORT      : %s\n", port);
    printf("SENS CONNECT ADDR   : %s\n", bind_addr);
    printf("META CONNECT ADDR   : %s\n", info_addr);

    printf("SERIAL_NUM       : %s\n", port);
    printf("CONTROLLER_VER   : %d\n", version);
    printf("BOARD_CHAIN      : %d\n", chain);

    // Storageノードから基板IDを取得
    std::vector<unsigned int> ids{};
    get_board_ids(info_addr, port,  chain, &ids, (unsigned int)version, (unsigned int)brd_master[version].modes);

    // create board instance
    std::vector<Board> brds;
    for(unsigned int id : ids){
        std::cout << "BID : " << id << std::endl;
        brds.emplace_back(id, brd_master[version].sensors, brd_master[version].modes);
    }

    // launch
    launch(port, bind_addr, color_addr, &brds);

}

int main(int argc, char* argv[]){

    int c;
    const char* optstring = "t:p:b:i:c:h:v:";

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

    int chain = 0;
    int version = 0;

    // get options
    while((c = getopt(argc, argv, optstring)) != -1){
        if(c == 'p') {
            ser_p = optarg;
        }else if(c == 'b') {
            bind_addr = optarg;
        }else if(c == 'i') {
            info_addr = optarg;
        }else if(c == 'c') {
            color_addr = optarg;
        }else if(c == 'h') {
            chain = atoi(optarg);
        }else if(c == 'v') {
            version = atoi(optarg);
        }else{
            // parse error
            printf("unknown argument error\n");
            return -1;
        }
    }

    // check for serial port
    if(ser_p == nullptr){
        printf("require port option -p");
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
    if(version == 0){
        printf("require board version");
        return -1;
    }
    if(chain == 0){
        printf("require chain count");
        return -1;
    }

    run(ser_p, bind_addr, info_addr, color_addr, version, chain);

    return 0;
}
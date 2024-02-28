//
// Created by ura on 6/26/23.
//


#include <iostream>

#include <opencv2/opencv.hpp>
#include <future>
#include <thread>
#include <queue>

#include <getopt.h>

#include "frame.h"
#include "mapper.h"
#include "utility.h"
#include "tracker.h"

#include "osc/OscOutboundPacketStream.h"
#include "ip/IpEndpointName.h"
#include "ip/UdpSocket.h"

cv::Scalar touch_color_arr[5] = {
        cv::Scalar(255, 0, 0),
        cv::Scalar(0, 255, 255),
        cv::Scalar(0, 0, 255),
        cv::Scalar(255, 255, 0),
        cv::Scalar(255, 0, 255)
};


bool running = false;
void run(const char *addr, const char *storage_addr, const char *com_addr);

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

    while(running){
        recv_msgs.clear();

        zmq::recv_multipart(subscriber, std::back_inserter(recv_msgs));
        if(recv_msgs.empty()) {
            std::cout << "timeout" << std::endl;
            continue;
        }
        if(recv_msgs.size() != 2) continue;

        int bid, mode;
        int ret = sscanf(recv_msgs.at(0).data<char>(), "BRD_DATA %d %d",
                         &bid,
                         &mode);
        if(ret == EOF) continue; //fail to decode

        unsigned long d_size = recv_msgs.at(1).size() / sizeof(uint16_t);
        update_frames(bid, mode, recv_msgs.at(1).data<uint16_t>(), d_size, frames);
    }
}

void command_handling(std::vector<frame> *frames, const char *addr){
    zmq::context_t ctx(1);
    zmq::socket_t subscriber(ctx, zmq::socket_type::rep);
    subscriber.bind(addr);

    std::vector<zmq::message_t> recv_msgs;

    while(running){
        recv_msgs.clear();

        zmq::recv_multipart(subscriber, std::back_inserter(recv_msgs));
        if(recv_msgs.empty()) {
            std::cout << "timeout" << std::endl;
            continue;
        }

        char command[256]{};
        int ret = sscanf(recv_msgs.at(0).data<char>(),
                         "ANALYZER %s",
                         command);
        if(ret == EOF) continue; // fail to decode

        // exec commands
        if(strcmp(command, "CAL_UPPER") == 0) {

            for(auto &frm : *frames){
                frm.set_upper();
            }

        }else if(strcmp(command, "CAL_LOWER") == 0){

            for(auto &frm : *frames){
                frm.set_lower();
            }

        }else if(strcmp(command, "RELOAD") == 0){
            running = false;

        }

        subscriber.send(zmq::buffer("ANALYZER ACK"), zmq::send_flags::none);

    }
}


void update(Mapper *me){
    UdpTransmitSocket sock(IpEndpointName("192.168.0.200", 9000));
    Tracker track(10, 2, 10, 5);

    while(running){
        me->update();

        cv::Mat tmp;
        me->get_img(tmp);

        // convert from 16bit value to 8bit value
        cv::Mat img8bit;
        tmp.convertTo(img8bit, CV_8UC1, 255.0 / (UINT16_MAX));

        // noise reduction
        /*for(int i = 0; i < img8bit.rows; i++){
            auto *m_ptr = img8bit.ptr<uint8_t>(i);
            for(int j = 0; j < img8bit.cols; j++){
                if(m_ptr[j] < 10) m_ptr[j] = 0;
            }
        }*/

        // rotate
        /*cv::Point2f center = cv::Point2f((img8bit.cols / 2  -1),(img8bit.rows / 2 -1));//図形の中心
        double degree = 45.0;  // 回転角度
        double scale = 1; //大きさの定義

        cv::Mat change = cv::getRotationMatrix2D(center, degree, scale); //回転&拡大縮小
        cv::warpAffine(img8bit, img8bit, change, img8bit.size(), cv::INTER_CUBIC,cv::BORDER_CONSTANT,cv::Scalar(0, 0, 0)); //画像の変換(アフィン変換)*/

        // interpolation
        cv::resize(img8bit, img8bit, cv::Size(), 4, 4, cv::INTER_CUBIC);

        // thresholding
        cv::Mat binary;
        cv::threshold(img8bit, binary, 0, 255, cv::THRESH_OTSU);

        // labeling
        cv::Mat LabelImg;
        cv::Mat stats;
        cv::Mat centroids;
        int nLab = cv::connectedComponentsWithStats(binary, LabelImg, stats, centroids);

        // find contours
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        cv::Mat result;
        cv::cvtColor(img8bit, result, cv::COLOR_GRAY2BGR);

        // update tracker
        cv::drawContours(result, contours, -1, cv::Scalar(0, 255, 0), 1);
        for (int i = 1; i < nLab; ++i) {
            double *param = centroids.ptr<double>(i);
            int x = static_cast<int>(param[0]);
            int y = static_cast<int>(param[1]);

            Object obj = {0, (float)x, (float)y, {}, 0};
            int id = track.update(&obj);
            if(id < 0) continue;
        }
        track.tick();

        // draw object point
        std::map<int, Object> objs;
        std::vector<int> eliminated;
        track.get_objs(&objs, &eliminated);

        for(auto obj : objs){
            cv::circle(result,cv::Point((int)obj.second.px, (int)obj.second.py), 1, touch_color_arr[obj.first], -1);

            //std::cout << " ( " << obj.first << ", " << obj.second.px << ", " << obj.second.py << " ), ";
        }
        //std::cout << std::endl;

        // translate
        for(auto obj : objs){
            float sx = obj.second.px -12;
            float sy = obj.second.py;
            float rx = (sx * cos(M_PI / 4 * -1)) - (sy * sin(M_PI / 4 * -1));
            float ry = (sx * sin(M_PI / 4 * -1)) + (sy * cos(M_PI / 4 * -1));
            obj.second.t_px = (int)rx;
            obj.second.t_py = (int)ry;
        }

        // publish osc
        char buffer[1024] = {};
        osc::OutboundPacketStream p(buffer, 1024);

        p << osc::BeginBundleImmediate;
        for(auto obj : objs){
            p << osc::BeginMessage((std::string("/touch/") + std::to_string(obj.first) + std::string("/point")).c_str())
              << static_cast<int32_t>(obj.second.t_px)
              << static_cast<int32_t>(obj.second.t_py)
              << osc::EndMessage;
        }
        for(auto elobj : eliminated){
            p << osc::BeginMessage((std::string("/touch/") + std::to_string(elobj) + std::string("/delete")).c_str())
              << static_cast<int32_t>(-1)
              << static_cast<int32_t>(-1)
              << osc::EndMessage;
        }
        p << osc::EndBundle;
        sock.Send(p.Data(), p.Size());

        // preview
        cv::resize(result, result, cv::Size(400, 400), 0, 0, cv::INTER_NEAREST);

        cv::Mat cm;
        cv::applyColorMap(img8bit, cm, cv::COLORMAP_JET);
        cv::resize(cm, cm, cv::Size(400, 400), 0, 0, cv::INTER_NEAREST);

        cv::imshow("test", result);
        cv::imshow("value", cm);
        cv::waitKey(10);

    }
}


void launch(std::vector<frame> *frms, const char *addr, const char *cm_addr, Mapper *mapper){
    running = true;

    std::future<void> recv_meta = std::async(std::launch::async, subscribe_data, frms, addr);
    std::future<void> img_update = std::async(std::launch::async, update, mapper);
    std::future<void> handing = std::async(std::launch::async, command_handling, frms, cm_addr);
    recv_meta.wait();
    img_update.wait();
    handing.wait();

}


void run(const char *addr, const char *storage_addr, const char *com_addr){

    std::cout << "Getting board data..." << std::endl;

    // get board layout from storage
    std::vector<Container> brds;
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

    launch(&frms, addr, com_addr, &mapper);

}

int main(int argc, char* argv[]){

    int c;
    const char* optstring = "d:b:i:";

    // enable error log from getopt
    opterr = 0;

    // bind address
    char* com_addr = nullptr;
    char* bind_addr = nullptr;
    char* storage_addr = nullptr;

    // get options
    while((c = getopt(argc, argv, optstring)) != -1){
        if(c == 'd') {
            storage_addr = optarg;
        }else if(c == 'b'){
            bind_addr = optarg;
        }else if(c == 'i') {
            com_addr = optarg;
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
    // check for com address
    if(com_addr == nullptr){
        printf("require command address option");
        return -2;
    }
    run(bind_addr, storage_addr, com_addr);

    return 0;
}

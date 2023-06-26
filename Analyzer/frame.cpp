//
// Created by ura on 6/26/23.
//

#include "frame.h"

void frame::pack_mat(cv::InputOutputArray io, tm_packet *pac, int sensors, const uint16_t *map) {
    int num = pac->s_num;
    int mode = pac->mode;

    if(num >= sensors)return;

    cv::Mat mat = io.getMat();
    int x = (map[num] >> 8) & 0xFF;
    int y = map[num] & 0xFF;
    x = x * 2;
    y = y * 2;

    switch(mode){
        case 1:
            mat.at<uint16_t>(y + 1, x) = pac->value;
            break;
        case 2:
            mat.at<uint16_t>(y + 1, x + 1) = pac->value;
            break;
        case 3:
            mat.at<uint16_t>(y, x) = pac->value;
            break;
        case 4:
            mat.at<uint16_t>(y, x + 1) = pac->value;
            break;
        default:
            break;
    }
    mat.copyTo(io);
}

void frame::gen_mat(cv::OutputArray dst) {
    cv::Mat fb = cv::Mat::zeros(cv::Size(20, 20), CV_16UC1);
    cv::Mat fb_l = cv::Mat::zeros(cv::Size(10, 10), CV_16UC1);
    cv::Mat show, show_l;

    std::vector<tm_packet> pacs = std::vector<tm_packet>();
    bool ret;

    uint16_t offset[121][5] = {};
    uint16_t r_max[121][5] = {};
    int calib_mode = 0;

    for (;;) {
        ret = ser.read(&pacs);
        for(tm_packet pac : pacs){
            if(calib_mode == 1)offset[pac.s_num][pac.mode] = pac.value;
            if(calib_mode == 2)r_max[pac.s_num][pac.mode] = pac.value;

            if(calib_mode != 3)continue;
            if(offset[pac.s_num][pac.mode] < 100) continue;

            if(offset[pac.s_num][pac.mode] < pac.value){
                pac.value -= offset[pac.s_num][pac.mode];
            }else{
                pac.value = 0;
            }

            float val = (float)pac.value / (float)(r_max[pac.s_num][pac.mode] - offset[pac.s_num][pac.mode]);
            if(val > 1.0f)val = 1.0;
            if(val < 0.0f)val = 0.0;

            pac.value = (uint16_t)(val * 65000);

            if(ret)pack_fb_mr(fb, &pac, 60, sens_map_tm3dis);
            if(ret && (pac.mode == 0))pack_fb(fb_l, &pac, 60, sens_map_tm3dis);
        }

        fb.convertTo(show, CV_8UC1, 256.0 /65536.0);
        cv::resize(show, show, cv::Size(40, 40), 0, 0, cv::INTER_CUBIC);
        cv::resize(show, show, cv::Size(256, 256), 0,0, cv::INTER_NEAREST);

        //cv::Sobel(show, show, CV_8UC1, 1, 0, 1, 1, 0, cv::BORDER_DEFAULT);

        //cv::resize(show, show, cv::Size(256, 256), 0,0, cv::INTER_NEAREST);
        cv::imshow("prev", show);

        fb_l.convertTo(show_l, CV_8UC1, 256.0 /65536.0);
        cv::resize(show_l, show_l, cv::Size(20, 20), 0, 0, cv::INTER_CUBIC);
        cv::resize(show_l, show_l, cv::Size(256, 256), 0,0, cv::INTER_NEAREST);
        cv::imshow("prev_l", show_l);

        int key = cv::waitKey(1);
        if(key == 'q'){
            cv::imwrite("save.png", show);
            cv::imwrite("save_lr.png", show_l);
            break;
        }
        switch (key) {
            case 'l':
                calib_mode = 1;
                break;
            case 'u':
                calib_mode = 2;
                break;
            case 'x':
                calib_mode = 3;
                break;
            default:
                break;
        }
    }

    ser.close();
}

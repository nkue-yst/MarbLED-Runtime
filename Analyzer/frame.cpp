//
// Created by ura on 6/26/23.
//

#include "frame.h"
#include "resources/sens_map_tm2.h"
#include "resources/sens_map_tm3dis.h"
#include "resources/sens_map_tm4.h"


frame::frame(board brd) {
    brd_data = brd;
    Brd_Master bm = get_brd_master();

    buf = std::vector<s_data>(brd.modes, s_data(bm.sensors));

    sensors = bm.sensors;
    f_buf_size = get_frame_size();
    f_buf = cv::Mat::zeros(f_buf_size, CV_16UC1);

    cal_upper = cv::Mat::zeros(f_buf_size, CV_16UC1);
    cal_lower = cv::Mat::zeros(f_buf_size, CV_16UC1);
    cal_gain = cv::Mat::zeros(f_buf_size, CV_16FC1);
}

uint16_t frame::get_id() const {
    return brd_data.id;
}

Brd_Master frame::get_brd_master() const {
    switch(brd_data.version){
        case 2:
            return tm2_master;
        case 3:
            return tm3_master;
        case 4:
            return tm4_master;
    }
    return Brd_Master{};
}

cv::Point2i frame::get_layout() {
    cv::Point2i cl(brd_data.layout_x, brd_data.layout_y);
    cv::Point2i tmp;
    led2sens_coordinate(&cl, &tmp);
    return tmp;
}

void frame::update(uint8_t mode, const uint16_t *data, unsigned long len) {
    if(len != sensors){
        std::cerr << "invalid data size (received:" << len << ", expected:" << sensors << ")" << std::endl;
        return;
    }
    for(int i = 0; i < len; i++){
        buf[mode][i] = data[i];
    }
}

void frame::pack_mat(const uint16_t *map) {
    if(buf.empty()) return;

    for(int i = 0; i < buf.size(); i++){
        for(int j = 0; j < buf.at(i).size(); j++){
            int x = (map[j] >> 8) & 0xFF;
            int y = map[j] & 0xFF;
            x = x * 2;
            y = y * 2;

            uint16_t val = buf.at(i).at(j);

            //if(y >= f_buf.rows | x >= f_buf.cols) continue;

            switch(i){
                case 1:
                    f_buf.at<uint16_t>(y + 1, x) = val;
                    break;
                case 2:
                    f_buf.at<uint16_t>(y + 1, x + 1) = val;
                    break;
                case 3:
                    f_buf.at<uint16_t>(y, x) = val;
                    break;
                case 4:
                    f_buf.at<uint16_t>(y, x + 1) = val;
                    break;
                default:
                    break;
            }
        }
    }
}

cv::Size2i frame::get_frame_size() {
    Brd_Master tmp = get_brd_master();
    return {(int)tmp.buf_x * 2, (int)tmp.buf_y * 2};
}

void frame::get_mat(cv::OutputArray dst) {
    pack_mat(get_brd_master().map);

    dst.create(f_buf_size, CV_16UC1);
    cv::Mat m = dst.getMat();

    for(int i = 0; i < f_buf_size.height; i++){
        auto *m_ptr = m.ptr<uint16_t>(i);
        auto *fb_ptr = f_buf.ptr<uint16_t>(i);
        for(int j = 0; j < f_buf_size.width; j++){
            m_ptr[j] = fb_ptr[j];
        }
    }

    //f_buf.copyTo(mat);
}

void frame::get_mat_calibrated(cv::OutputArray dst){
    dst.create(f_buf_size, CV_8UC1);
    cv::Mat m = dst.getMat();

    cv::Mat tmp;
    get_mat(tmp);

    // remove offset
    for(int i = 0; i < f_buf_size.height; i++){
        auto *cl_ptr = cal_lower.ptr<uint16_t>(i);
        auto *fb_ptr = tmp.ptr<uint16_t>(i);
        for(int j = 0; j < f_buf_size.width; j++){
            if(fb_ptr[j] > cl_ptr[j]){
                fb_ptr[j] -= cl_ptr[j];
            }else{
                fb_ptr[j] = 0x00;
            }
        }
    }

    // calc range
    for(int i = 0; i < f_buf_size.height; i++){
        auto *cg_ptr = cal_gain.ptr<float>(i);
        auto *fb_ptr = tmp.ptr<uint16_t>(i);
        auto *dst_ptr = m.ptr<uint8_t>(i);
        for(int j = 0; j < f_buf_size.width; j++){
            dst_ptr[j] = (float)fb_ptr[j] * (float)cg_ptr[j];
        }
    }
}

void frame::led2sens_coordinate(const cv::Point2i *src, cv::Point2i *dst) const {
    switch(brd_data.version){
        case 4:
            double sx = (double)src->x / 18 * 6;
            double sy = (double)src->y / 18 * 6;
            double x = (sx * cos(M_PI / 4)) - (sy * sin(M_PI / 4));
            double y = (sx * sin(M_PI / 4)) + (sy * cos(M_PI / 4));
            dst->x = floor(x);
            dst->y = floor(y);
            break;
    }
}

void frame::calc_gain(){
    for(int i = 0; i < f_buf_size.height; i++){
        auto *cl_ptr = cal_lower.ptr<uint16_t>(i);
        auto *cu_ptr = cal_upper.ptr<uint16_t>(i);
        auto *cg_ptr = cal_gain.ptr<float>(i);
        for(int j = 0; j < f_buf_size.width; j++){
            if(cu_ptr[j] > cl_ptr[j]){
                cg_ptr[j] = 255.0 / (float)(cu_ptr[j] - cl_ptr[j]);
            }else{
                cg_ptr[j] = 0.0;
            }
        }
    }
}

void frame::set_lower() {
    for(int i = 0; i < f_buf_size.height; i++){
        auto *m_ptr = cal_lower.ptr<uint16_t>(i);
        auto *fb_ptr = f_buf.ptr<uint16_t>(i);
        for(int j = 0; j < f_buf_size.width; j++){
            m_ptr[j] = fb_ptr[j];
        }
    }
    calc_gain();
}

void frame::set_upper() {
    for(int i = 0; i < f_buf_size.height; i++){
        auto *m_ptr = cal_upper.ptr<uint16_t>(i);
        auto *fb_ptr = f_buf.ptr<uint16_t>(i);
        for(int j = 0; j < f_buf_size.width; j++){
            m_ptr[j] = fb_ptr[j];
        }
    }
    calc_gain();
}

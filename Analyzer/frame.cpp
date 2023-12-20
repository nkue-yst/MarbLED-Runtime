//
// Created by ura on 6/26/23.
//

#include "frame.h"
#include "resources/sens_map_tm2.h"
#include "resources/sens_map_tm3dis.h"
#include "resources/sens_map_tm4.h"


frame::frame(Container brd) {
    brd_data = brd;
    Brd_Master bm = get_brd_master();

    buf = std::vector<s_data>(brd.modes, s_data(bm.sensors));

    sensors = bm.sensors;
    f_buf_size = get_frame_size();
    f_buf = cv::Mat::zeros(f_buf_size, CV_16UC1);

    cal_lower = std::vector<s_data>(brd.modes, s_data(bm.sensors));
    cal_upper = std::vector<s_data>(brd.modes, s_data(bm.sensors));
    cal_gain = std::vector<std::vector<float>>(brd.modes, std::vector<float>(bm.sensors));
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

void frame::pack_mat(const uint16_t *map, const std::vector<s_data> *cbuf) {
    if(cbuf->empty()) return;

    // mode
    for(int i = 0; i < cbuf->size(); i++){
        // sensor
        for(int j = 0; j < cbuf->at(i).size(); j++){
            int x = (map[j] >> 8) & 0xFF;
            int y = map[j] & 0xFF;

            if(brd_data.modes == 5) {
                x = x * 2;
                y = y * 2;
            }

            uint16_t val = cbuf->at(i).at(j);

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
                    f_buf.at<uint16_t>(y, x) = val;
                    break;
            }
        }
    }
}

cv::Size2i frame::get_frame_size() {
    Brd_Master tmp = get_brd_master();
    if(brd_data.modes == 5){
        return {(int)tmp.buf_x * 2, (int)tmp.buf_y * 2};
    }else{
        return {(int)tmp.buf_x, (int)tmp.buf_y};
    }
}

void frame::get_mat(cv::OutputArray dst) {
    pack_mat(get_brd_master().map, &buf);

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

    std::vector<s_data> cal_d(brd_data.modes, s_data(sensors));
    std::copy(buf.begin(), buf.end(), cal_d.begin());

    // remove offset
    for(int mode = 0; mode < buf.size(); mode++){
        for(int s = 0; s < buf.at(mode).size(); s++){

            // cancel offset
            if(cal_d[mode][s] > cal_lower[mode][s]){
                cal_d.at(mode).at(s) -= cal_lower[mode][s];
            }else{
                cal_d.at(mode).at(s) = 0.0;
            }

            // calc range
            unsigned int tmp = cal_d.at(mode).at(s) * cal_gain.at(mode).at(s);
            cal_d.at(mode).at(s) = (tmp > UINT16_MAX) ? UINT16_MAX : (uint16_t)tmp;
        }
    }

    pack_mat(get_brd_master().map, &cal_d);

    dst.create(f_buf_size, CV_16UC1);
    cv::Mat m = dst.getMat();

    for(int i = 0; i < f_buf_size.height; i++){
        auto *m_ptr = m.ptr<uint16_t>(i);
        auto *fb_ptr = f_buf.ptr<uint16_t>(i);
        for(int j = 0; j < f_buf_size.width; j++){
            m_ptr[j] = fb_ptr[j];
        }
    }
}

void frame::led2sens_coordinate(const cv::Point2i *src, cv::Point2i *dst) const {
    switch(brd_data.version){
        case 4:
            double sx = (double)src->x / 18 * 5;
            double sy = (double)src->y / 18 * 5;
            double x = (sx * cos(M_PI / 4)) - (sy * sin(M_PI / 4));
            double y = (sx * sin(M_PI / 4)) + (sy * cos(M_PI / 4));
            dst->x = floor(x);
            dst->y = floor(y);
            break;
    }
}

void frame::calc_gain(){
    std::cout << "[ cal length ] bid : " << brd_data.id << std::endl;
    for(int mode = 0; mode < buf.size(); mode++){
        std::cout << "-- mode : " << mode << " --" << std::endl;
        for(int s = 0; s < buf.at(mode).size(); s++){
            if(cal_upper[mode][s] > cal_lower[mode][s]){
                cal_gain.at(mode).at(s) = (float)UINT16_MAX / (float)(cal_upper[mode][s] - cal_lower[mode][s]);
            }else{
                cal_gain.at(mode).at(s) = 0.0;
            }
            std::cout << cal_upper[mode][s] - cal_lower[mode][s] << " ";
        }
        std::cout << std::endl;
    }
}

void frame::set_lower() {
    std::copy(buf.begin(), buf.end(), cal_lower.begin());
    calc_gain();
}

void frame::set_upper() {
    std::copy(buf.begin(), buf.end(), cal_upper.begin());
    calc_gain();
}

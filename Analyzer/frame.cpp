//
// Created by ura on 6/26/23.
//

#include "frame.h"
#include "resources/sens_map_tm2.h"
#include "resources/sens_map_tm3dis.h"
#include "resources/sens_map_tm4.h"


frame::frame(board brd) {
    brd_data = brd;
    buf = std::vector<s_data>(brd.modes, s_data(brd.version));

    Brd_Master bm = get_brd_master();
    f_buf_size = cv::Size2i(bm.buf_x, bm.buf_y);
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
}

cv::Point2i frame::get_layout() {
    cv::Point2i cl(brd_data.layout_x, brd_data.layout_y);
    cv::Point2i tmp;
    led2sens_coordinate(&cl, &tmp);
    return tmp;
}

void frame::update(uint8_t mode, const uint16_t *data) {
    int n = sizeof(&data) / sizeof(data[0]);
    buf[mode] = s_data(data, data + n);
}

void frame::pack_mat(const uint16_t *map) {
    cv::Mat mat = cv::Mat::zeros(f_buf_size, CV_16UC1);

    for(int i = 0; i < buf.size(); i++){
        for(int j = 0; j < buf.at(i).size(); j++){
            int x = (map[j] >> 8) & 0xFF;
            int y = map[j] & 0xFF;
            x = x * 2;
            y = y * 2;

            uint16_t val = buf.at(i).at(j);

            switch(i){
                case 1:
                    mat.at<uint16_t>(y + 1, x) = val;
                    break;
                case 2:
                    mat.at<uint16_t>(y + 1, x + 1) = val;
                    break;
                case 3:
                    mat.at<uint16_t>(y, x) = val;
                    break;
                case 4:
                    mat.at<uint16_t>(y, x + 1) = val;
                    break;
                default:
                    break;
            }
        }
    }

    mat.copyTo(f_buf);
}

cv::Size2i frame::get_frame_size() {
    Brd_Master tmp = get_brd_master();
    return {(int)tmp.buf_x, (int)tmp.buf_y};
}

void frame::get_mat(cv::OutputArray dst) {
    pack_mat(get_brd_master().map);
    f_buf.copyTo(dst);
}

void frame::led2sens_coordinate(const cv::Point2i *src, cv::Point2i *dst) {
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

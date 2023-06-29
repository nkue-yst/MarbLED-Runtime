//
// Created by ura on 6/26/23.
//

#include "frame.h"
#include "resources/sens_map_tm2.h"
#include "resources/sens_map_tm3dis.h"


frame::frame(board brd) {
    brd_data = brd;
    buf = std::vector<s_data>(brd.modes, s_data(brd.sensors));
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

void frame::get_mat(cv::OutputArray dst) {
    switch(brd_data.version){
        case 2:
            pack_mat(sens_map_tm2);
            break;
        case 3:
            pack_mat(sens_map_tm3dis);
            break;
        default:
            break;
    }
    f_buf.copyTo(dst);
}

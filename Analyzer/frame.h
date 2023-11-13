//
// Created by ura on 6/26/23.
//

#ifndef TOUCHMATRIX_ANALYZER_FRAME_H
#define TOUCHMATRIX_ANALYZER_FRAME_H

#include <opencv2/opencv.hpp>

#include "zmq.hpp"
#include "zmq_addon.hpp"

#include "utility.h"

#include "resources/board_template.h"

typedef std::vector<uint16_t> s_data;

class frame {

private:
    Container brd_data{};           // board meta data
    std::vector<s_data> buf;    // buffer of sensing data
    cv::Mat f_buf;              // buffer of mapped image

    int sensors;
    cv::Size2i f_buf_size;

    std::vector<s_data> cal_lower;
    std::vector<s_data> cal_upper;
    std::vector<std::vector<float>> cal_gain;

    void pack_mat(const uint16_t *map, const std::vector<s_data> *cbuf);
    void led2sens_coordinate(const cv::Point2i *src, cv::Point2i *dst) const;
    Brd_Master get_brd_master() const;
    void calc_gain();

public:
    explicit frame(Container brd);

    uint16_t get_id() const;
    cv::Point2i get_layout();
    cv::Size2i get_frame_size();
    void get_mat(cv::OutputArray dst);
    void get_mat_calibrated(cv::OutputArray dst);

    void set_lower();
    void set_upper();
    void load_calibration_data();

    void update(uint8_t mode, const uint16_t *data, unsigned long len);
};


#endif //TOUCHMATRIX_ANALYZER_FRAME_H

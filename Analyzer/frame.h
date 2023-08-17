//
// Created by ura on 6/26/23.
//

#ifndef TOUCHMATRIX_ANALYZER_FRAME_H
#define TOUCHMATRIX_ANALYZER_FRAME_H

#include <opencv2/opencv.hpp>

#include "zmq.hpp"
#include "zmq_addon.hpp"

typedef std::vector<uint16_t> s_data;

struct board{
    char serial[256];
    uint16_t id;
    uint16_t controller_id;
    uint8_t version;
    uint8_t chain_num;
    uint8_t modes;
    int32_t layout_x;
    int32_t layout_y;
};


class frame {

private:
    board brd_data{};
    std::vector<s_data> buf;
    cv::Mat f_buf;

    cv::Point2i place;
    cv::Size2i f_buf_size;

    void pack_mat(const uint16_t *map);

public:
    explicit frame(board brd);
    void get_mat(cv::OutputArray dst);

    void update(uint8_t mode, const uint16_t *data);
};


#endif //TOUCHMATRIX_ANALYZER_FRAME_H

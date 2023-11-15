//
// Created by chihiro on 23/06/28.
//

#ifndef TOUCHMATRIX_ANALYZER_MAPPER_H
#define TOUCHMATRIX_ANALYZER_MAPPER_H


#include "frame.h"


class Mapper {

private:
    std::vector<frame> *frms;
    cv::Mat fb;

    cv::Size2i calc_fb_size();
    void place_mat(cv::Point2i p, const cv::Mat *src);

public:
    explicit Mapper(std::vector<frame> *frames);
    void update();
    void get_img(cv::OutputArray dst);

};


#endif //TOUCHMATRIX_ANALYZER_MAPPER_H

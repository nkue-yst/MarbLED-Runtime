//
// Created by chihiro on 23/06/28.
//

#include "mapper.h"


Mapper::Mapper(std::vector<frame> *frames){
    frms = frames;

    cv::Size2i fb_size = calc_fb_size();
    fb = cv::Mat::zeros(fb_size.width, fb_size.height, CV_16UC1);
}

cv::Size2i Mapper::calc_fb_size() {
    int minx = INT_MAX, maxx = 0;
    int miny = INT_MAX, maxy = 0;

    for(auto frm : *frms){
        cv::Point2i tmp = frm.get_layout();
        cv::Size2i st = frm.get_frame_size();

        if(minx > tmp.x) minx = tmp.x;
        if(maxx < tmp.x + st.width) maxx = tmp.x + st.width;

        if(miny > tmp.y) miny = tmp.y;
        if(maxy < tmp.y + st.height) maxy = tmp.y + st.height;
    }

    return {maxx - minx, maxy - miny};
}

void Mapper::place_mat(cv::Point2i p, const cv::Mat *src) {
    for(int i = 0; i < src->cols; i++){
        for(int j = 0; j < src->rows; j++){
            if( p.x + i > fb.cols | p.y + j > fb.rows) continue;
            fb.at<uint16_t>(p.y + j, p.x + i) = src->at<uint16_t>(j, i);
        }
    }
}

void Mapper::update() {

    cv::Mat p;
    for(auto frm : *frms){
        frm.get_mat(p);
        place_mat(frm.get_layout(), &p);
    }
    cv::Mat tmp;
    cv::resize(p, tmp, cv::Size(100, 100), 30, 30, cv::INTER_NEAREST);
    cv::imshow("prev", tmp);
    cv::waitKey(100);
}

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
        if(maxx < tmp.x + st.width) maxx = tmp.x + st.width + 1;

        if(miny > tmp.y) miny = tmp.y;
        if(maxy < tmp.y + st.height) maxy = tmp.y + st.height + 1;
    }

    return {maxx - minx, maxy - miny};
}

void Mapper::place_mat(cv::Point2i p, const cv::Mat *src) {
    for(int i = 0; i < src->cols; i++){
        for(int j = 0; j < src->rows; j++){
            if( p.x + i > fb.cols | p.y + j > fb.rows) continue;
            fb.at<uint16_t>(p.y + j, p.x + i) += src->at<uint16_t>(j, i);
        }
    }
}

void Mapper::update() {

    cv::Mat p;
    fb = cv::Mat::zeros(fb.rows, fb.cols, CV_16UC1);
    for(auto frm : *frms){
        frm.get_mat_calibrated(p);
        place_mat(frm.get_layout(), &p);
    }

}

void Mapper::get_img(cv::OutputArray dst) {
    dst.create(fb.rows, fb.cols, CV_16UC1);
    cv::Mat m = dst.getMat();

    for(int i = 0; i < fb.rows; i++){
        auto *m_ptr = m.ptr<uint16_t>(i);
        auto *fb_ptr = fb.ptr<uint16_t>(i);
        for(int j = 0; j < fb.cols; j++){
            m_ptr[j] = fb_ptr[j];
        }
    }
}

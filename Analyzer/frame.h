//
// Created by ura on 6/26/23.
//

#ifndef TOUCHMATRIX_ANALYZER_FRAME_H
#define TOUCHMATRIX_ANALYZER_FRAME_H

#include <opencv2/opencv.hpp>

#include "zmq.hpp"
#include "zmq_addon.hpp"

class frame {
private:
    char *serial;
    int modes;
    std::vector<std::vector<uint16_t >> buf;

    void pack_mat();
    static void store_buffer(const char *serial, int mode, const uint16_t *data);

public:
    frame(const char* s);
    void gen_mat(cv::OutputArray dst);

    static void zmq_receive(const char *addr);
};


#endif //TOUCHMATRIX_ANALYZER_FRAME_H

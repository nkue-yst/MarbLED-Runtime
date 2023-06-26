//
// Created by ura on 6/26/23.
//

#ifndef TOUCHMATRIX_ANALYZER_FRAME_H
#define TOUCHMATRIX_ANALYZER_FRAME_H

#include <opencv2/opencv.hpp>

class frame {
private:
    char *serial;
    int modes;
    std::vector<std::vector<uint16_t >> buf;

    void pack_mat();

public:
    frame();
    void gen_mat(cv::OutputArray dst);
};


#endif //TOUCHMATRIX_ANALYZER_FRAME_H

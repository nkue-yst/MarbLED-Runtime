//
// Created by chihiro on 23/06/28.
//

#ifndef TOUCHMATRIX_ANALYZER_MAPPER_H
#define TOUCHMATRIX_ANALYZER_MAPPER_H


#include "frame.h"


class Mapper {

private:
    std::map<char*, frame> frames;

    static void local_key(char *merged_key, const char *serial, int c_num);

public:
    Mapper();
    void set_connected_boards(const std::vector<board>& boards);
    void update(const char *serial, int c_num, int mode, const uint16_t *data);

};


#endif //TOUCHMATRIX_ANALYZER_MAPPER_H

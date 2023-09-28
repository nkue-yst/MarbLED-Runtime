//
// Created by chihiro on 23/08/27.
//

#ifndef MARBLED_SENSING_BOARD_TEMPLATE_H
#define MARBLED_SENSING_BOARD_TEMPLATE_H

typedef struct{
    unsigned int sensors;
    unsigned int buf_x;
    unsigned int buf_y;
    uint16_t map[256];
} Brd_Master;

#endif //MARBLED_SENSING_BOARD_TEMPLATE_H

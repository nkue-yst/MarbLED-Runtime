//
// Created by chihiro on 23/09/10.
//

#ifndef MARBLED_SENSING_SENS_MAP_TM4_H
#define MARBLED_SENSING_SENS_MAP_TM4_H

#include "board_template.h"

Brd_Master tm4_master{
        18,
        6,
        5,
        {
                (5 << 8) | 0,
                (6 << 8) | 1,
                (7 << 8) | 2,
                (8 << 8) | 3,
                (9 << 8) | 4,

                (4 << 8) | 0,
                (5 << 8) | 1,
                (6 << 8) | 2,
                (7 << 8) | 3,
                (8 << 8) | 4,
                (9 << 8) | 5,

                (4 << 8) | 1,
                (5 << 8) | 2,
                (6 << 8) | 3,
                (7 << 8) | 4,
                (8 << 8) | 5,

                (3 << 8) | 1,
                (4 << 8) | 2
        }
};

#endif //MARBLED_SENSING_SENS_MAP_TM4_H

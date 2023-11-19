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
                (3 << 8) | 0,
                (4 << 8) | 1,
                (5 << 8) | 2,
                (2 << 8) | 0,
                (3 << 8) | 1,

                (4 << 8) | 2,
                (2 << 8) | 1,
                (3 << 8) | 2,
                (4 << 8) | 3,
                (1 << 8) | 1,
                (2 << 8) | 2,

                (3 << 8) | 3,
                (1 << 8) | 2,
                (2 << 8) | 3,
                (3 << 8) | 4,
                (0 << 8) | 2,

                (1 << 8) | 3,
                (2 << 8) | 4
        }
};

#endif //MARBLED_SENSING_SENS_MAP_TM4_H

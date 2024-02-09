//
// Created by ura on 24/01/17.
//

#ifndef MARBLED_RUNTIME_TRACKER_H
#define MARBLED_RUNTIME_TRACKER_H


#include <vector>
#include <cstdint>

typedef struct object{
    uint8_t id;
    int32_t px;
    int32_t py;
    uint8_t contour[32];
} Object;


class Tracker {
private:
    std::vector<Object> objs;
    int distance_th = 15;
    int eliminate_tick = 4;
    int maximum_obj = 10;
    int count = 0;

public:
    Tracker(int dis, int el_tick, int max);
    int update(Object obj);
    void tick();

};


#endif //MARBLED_RUNTIME_TRACKER_H

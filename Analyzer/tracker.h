//
// Created by ura on 24/01/17.
//

#ifndef MARBLED_RUNTIME_TRACKER_H
#define MARBLED_RUNTIME_TRACKER_H


#include <vector>
#include <cstdint>
#include <map>

typedef struct object{
    uint8_t id;
    float px;
    float py;
    uint8_t contour[32];
    uint8_t life_time;
    bool candidate;
    uint8_t candidate_time;
} Object;


class Tracker {
private:
    std::map<int, Object> objs;
    std::map<int, object> prev_objs;
    int distance_th = 15;
    int eliminate_tick = 4;
    int raise_tick = 4;
    int maximum_obj = 10;
    int count = 0;

    int nearest_object(Object src);
    int not_used_id();
    void eliminate();
    void spend();
    void raise();

public:
    Tracker(int dis, int el_tick, int raise, int max);
    int update(Object *obj);
    void tick();
    std::map<int, Object> * get_objs();

};


#endif //MARBLED_RUNTIME_TRACKER_H

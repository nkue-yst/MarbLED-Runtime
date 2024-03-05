//
// Created by ura on 24/01/17.
//

#include <cmath>
#include <iostream>
#include "tracker.h"

// Calc distance between Objects
double calc_distance(Object obj0, Object obj1){
    return std::sqrt((obj0.px - obj1.px) * (obj0.px - obj1.px) + (obj0.py - obj1.py) * (obj0.py - obj1.py));
}

// Search nearest object in object list
int Tracker::nearest_object(Object src){
    int32_t min = INT32_MAX;
    std::pair<int, Object> nearest;

    for(auto obj : objs){
        double dis = calc_distance(src, obj.second);
        auto idis = int(dis);

        if(min > idis){
            min = idis;
            nearest = obj;
        }

    }
    if(min > distance_th) return -1;
    if(min == INT32_MAX) return -1;

    return nearest.first;
}

// return not used id
int Tracker::not_used_id(){
    for(int i = 0; i < maximum_obj; i++){
        if(objs.find(i) == objs.end()) return i;
    }
    // detected obj has reached maximum
    return -1;
}

void Tracker::eliminate() {
    std::vector<int> eliminate_list;
    for(auto &obj : objs){
        if(obj.second.life_time > eliminate_tick) eliminate_list.emplace_back(obj.first);
    }

    last_eliminated = eliminate_list;

    for(auto id : eliminate_list){
        objs.erase(id);
        prev_objs.erase(id);
    }
}

void Tracker::spend(){
    for(auto &obj : objs){
        obj.second.life_time++;
    }
}

void Tracker::raise() {
    for(auto &obj : objs){
        if(obj.second.candidate_time > raise_tick && obj.second.candidate) {
            obj.second.life_time = 0;
            obj.second.candidate_time = 0;
            obj.second.candidate = false;

            prev_objs.insert(obj);
        }
    }
}

Tracker::Tracker(int dis, int el_tick, int raise, int max) {
    distance_th = dis;
    eliminate_tick = el_tick;
    maximum_obj = max;
    raise_tick = raise;
}

int Tracker::update(Object *obj) {
    int id = nearest_object(*obj);

    // nearest object not found.
    if(id < 0){
        obj->candidate = true;
        id = not_used_id();
        if(id < 0) return -1;
    }else{
        if(objs.at(id).candidate){
            obj->candidate = true;
            obj->candidate_time = objs.at(id).candidate_time + 1;
        }
        objs.erase(id);
    }

    obj->id = id;

    objs.insert(std::make_pair(id, *obj));
    return id;
}

void Tracker::tick() {
    count++;
    eliminate();
    raise();
    spend();
}

// get smoothed objects
void Tracker::get_objs(std::map<int, Object> *objs_, std::vector<int> *eliminated){
    float gain = 0.2;

    for(auto &obj : prev_objs){
        float diff_x = objs.at(obj.first).px - obj.second.px;
        float diff_y = objs.at(obj.first).py - obj.second.py;
        float mv_x = diff_x * gain;
        float mv_y = diff_y * gain;
        obj.second.px = obj.second.px + mv_x;
        obj.second.py = obj.second.py + mv_y;
    }

    *objs_ = prev_objs;
    *eliminated = last_eliminated;
}
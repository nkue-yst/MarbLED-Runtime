//
// Created by chihiro on 23/10/11.
//

#ifndef MARBLED_RUNTIME_BOARD_H
#define MARBLED_RUNTIME_BOARD_H

#include <iostream>
#include <queue>
#include <vector>
#include <string>
#include <thread>
#include <termios.h>
#include <future>
#include <cmath>
#include <map>


#define     BUFFER_MAX       10

typedef std::vector<std::vector<uint8_t>> f_color;
typedef std::vector<std::vector<uint16_t>> f_img;

class Board {

private:
    unsigned int id;
    uint8_t sensors;
    uint8_t modes;

    std::queue<f_color> color_queue;
    std::queue<f_img> data_queue;

    template<typename T>
    static void store_buffer(const T& frm, std::queue<T>& q);

public:

    Board(unsigned int id, uint8_t sensors, uint8_t modes);
    unsigned int get_id();
    uint8_t get_sensors();
    uint8_t get_modes();
    void store_sensor(const f_img *src);
    void store_color(const f_color *src);

};


#endif //MARBLED_RUNTIME_BOARD_H

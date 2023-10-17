//
// Created by chihiro on 23/10/11.
//

#include "board.h"


Board::Board(unsigned int id, uint8_t sensors, uint8_t modes) {
    this->id = id;
    this->sensors = sensors;
    this->modes = modes;
}

template<typename T>
void Board::store_buffer(const T& frm, std::queue<T>& q){
    // block when fifo buffer is max
    while(q.size() >= BUFFER_MAX){
        std::cerr << "buffer is max" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    q.push(frm);
}

unsigned int Board::get_id() {
    return this->id;
}

uint8_t Board::get_sensors() {
    return this->sensors;
}

uint8_t Board::get_modes() {
    return this->modes;
}

void Board::store_sensor(const f_img *src) {
    store_buffer(*src, data_queue);
}

void Board::store_color(const f_color *src) {
    store_buffer(*src, color_queue);
}

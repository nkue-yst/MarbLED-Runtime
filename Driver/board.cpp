//
// Created by chihiro on 23/10/11.
//

#include "board.h"

std::map<unsigned int, brd_config> brd_master{
        {4,{18, 288, 18, 18, 1}}
};

Board::Board(unsigned int id, uint8_t sensors, uint8_t modes) {
    this->id = id;
    this->sensors = sensors;
    this->modes = modes;
}

template<typename T>
void Board::store_buffer(const T& frm, std::queue<T>& q){
    // block when fifo buffer is max
    while(q.size() >= BUFFER_MAX){
        std::cerr << "buffer is max. I'm board no." << id << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    q.push(frm);
}

template <typename T>
int Board::pop_buffer(T &frm, std::queue<T> &q) {
    // retry when fifo is empty
    if(q.empty()){
        return -1;
    }

    // get from fifo buffer
    T frame = q.front();
    //if(q.size() != 1) q.pop();
    q.pop();

    std::copy(frame.begin(), frame.end(),frm.begin());
    return 0;
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

int Board::pop_sensor_values(f_img *dst) {
    int ret = pop_buffer(*dst, data_queue);
    return ret;
}

int Board::pop_color_values(f_color *dst) {
    int ret = pop_buffer(*dst, color_queue);
    return ret;
}

void Board::store_sensor(const f_img *src) {
    store_buffer(*src, data_queue);
}

void Board::store_color(const f_color *src) {
    store_buffer(*src, color_queue);
}

//
// Created by chihiro on 23/06/28.
//

#include "mapper.h"

Mapper::Mapper() = default;

void Mapper::local_key(char *merged_key, const char *serial, int c_num) {
    std::snprintf(merged_key, 256, "%s_%d", serial, c_num);
}

void Mapper::update(const char *serial, int c_num, int mode, const uint16_t *data) {
    char key[256]{};
    local_key(key, serial, c_num);

    size_t count = frames.count(key);
    if(count != 0){
        frames.at(key).update(mode, data);
    }
}

void Mapper::set_connected_boards(const std::vector<board>& boards) {
    frames.clear();

    for(board brd: boards){
        char key[256]{};
        frame f(brd);

        local_key(key, brd.serial, brd.version);
        frames.emplace(key, f);
    }
}

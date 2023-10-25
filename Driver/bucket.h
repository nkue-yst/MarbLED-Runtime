//
// Created by chihiro on 23/10/12.
//

#ifndef MARBLED_RUNTIME_BUCKET_H
#define MARBLED_RUNTIME_BUCKET_H

#include <iostream>
#include <vector>

struct tm_packet{
    uint16_t d_num;
    uint16_t value;
};

class Bucket {
public:
    virtual int tm_open(){};
    virtual int read(std::vector<tm_packet> *pacs){};
    virtual void transfer(const uint8_t *color, size_t len){};
    virtual void close() const{};
};


#endif //MARBLED_RUNTIME_BUCKET_H

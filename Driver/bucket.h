//
// Created by chihiro on 23/10/12.
//

#ifndef MARBLED_RUNTIME_BUCKET_H
#define MARBLED_RUNTIME_BUCKET_H

#include <iostream>
#include <vector>

struct tm_packet{
    uint8_t s_num;
    uint8_t mode;
    uint16_t value;
};

class Bucket {
public:
    virtual int tm_open(){};
    virtual int read(std::vector<tm_packet> *pacs){};
    virtual void close() const{};
};


#endif //MARBLED_RUNTIME_BUCKET_H

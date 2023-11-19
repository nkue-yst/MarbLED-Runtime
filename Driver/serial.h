//
// Created by ura on 11/17/22.
//

#ifndef TOUCHMATRIX_DRIVER_SERIAL_H
#define TOUCHMATRIX_DRIVER_SERIAL_H

#include "bucket.h"

// slip protocol defines
#define SLIP_END        0xC0
#define SLIP_ESC        0xDB
#define SLIP_ESC_END    0xDC
#define SLIP_ESC_ESC    0xDD


class serial : public Bucket {
private:
    unsigned char buf[1024];

    const char *file;
    int baudRate;

    int fd = -1;
    std::vector<unsigned char> packet;
    bool end_flg = false;
    bool esc_flg = false;

public:
    serial(const char *file, int baudRate);
    int tm_open() override;
    int read(std::vector<tm_packet> *pacs) override;
    void transfer(const uint8_t *color, size_t len) override;
    void close() const override;
    ~serial();

private:
    bool decodeSlip(long len, std::vector<tm_packet> *pacs);
    void generatePacket(tm_packet *pac);

};

#endif //TOUCHMATRIX_DRIVER_SERIAL_H

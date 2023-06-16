//
// Created by ura on 11/17/22.
//

#ifndef TOUCHMATRIX_DRIVER_SERIAL_H
#define TOUCHMATRIX_DRIVER_SERIAL_H


// slip protocol defines
#define SLIP_END        0xC0
#define SLIP_ESC        0xDB
#define SLIP_ESC_END    0xDC
#define SLIP_ESC_ESC    0xDD

struct tm_packet{
    uint8_t s_num;
    uint8_t mode;
    uint16_t value;
};

static void printPacket(const tm_packet *pac){
    printf("num: %d mode: %d data: %d\n", pac->s_num, pac->mode, pac->value);
}

class serial {
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
    int tm_open();
    int read(std::vector<tm_packet> *pacs);
    void close() const;
    ~serial();

private:
    bool decodeSlip(long len, std::vector<tm_packet> *pacs);
    void generatePacket(tm_packet *pac);

};

#endif //TOUCHMATRIX_DRIVER_SERIAL_H

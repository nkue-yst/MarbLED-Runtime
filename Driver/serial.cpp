//
// Created by ura on 11/17/22.
//

#include <iostream>
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#include <csignal>
#include <vector>

#include "serial.h"

serial::serial(const char *file, int baudRate) {
    packet = std::vector<unsigned char>(4);
    serial::file = file;
    serial::baudRate = baudRate;
}

serial::~serial() {
    close();
}

int serial::tm_open() {
    struct termios tio{};

    fd = open(file, O_RDWR);
    if (fd < 0) {
        std::cout << "serial open error" << std::endl;
        return -1;
    }

    tio.c_cflag += CREAD;   // read enable
    tio.c_cflag += CLOCAL;  // local-line (without modem flow)
    tio.c_cflag += CS8;     // data-bit 8bit
    tio.c_cflag += 0;       // stop-bit 1bit
    tio.c_cflag += 0;       // parity disable

    cfsetispeed(&tio, baudRate);
    cfsetospeed(&tio, baudRate);

    cfmakeraw(&tio);

    tcsetattr(fd, TCSANOW, &tio);
    ioctl(fd, TCSETS, &tio);

    return 0;
}

void serial::generatePacket(tm_packet *pac) {
    pac->d_num = (packet[0] << 8) | packet[1];
    pac->value = (packet[2] << 8) | packet[3];
}

bool serial::decodeSlip(long len, std::vector<tm_packet> *pacs) {
    bool packet_receive_flg = false;

    tm_packet pac{};
    pacs->clear();

    for (int i = 0; i < len; i++) {

        if (packet.size() > 4) end_flg = false;

        switch (buf[i]) {
            case SLIP_END:  // detect end packet
                end_flg = true;
                packet_receive_flg = true;
                generatePacket(&pac);
                pacs->push_back(pac);
                packet.clear();
                break;

            case SLIP_ESC:  // detect esc packet
                // if escape end-packet or esc-packet
                if (end_flg | esc_flg) esc_flg = true;
                break;

            case SLIP_ESC_END:  // detect end-escape end
                if (end_flg & esc_flg) {
                    packet.push_back(SLIP_END);
                    esc_flg = false;
                    break;
                }
                packet.push_back(SLIP_ESC_END);
                break;

            case SLIP_ESC_ESC:  // detect esc-escape end
                if (esc_flg) {
                    packet.push_back(SLIP_ESC);
                    esc_flg = false;
                    break;
                }
                packet.push_back(SLIP_ESC_ESC);
                break;

            default:
                if (end_flg) packet.push_back(buf[i]);
                break;
        }
    }

    return packet_receive_flg;
}

int serial::read(std::vector<tm_packet> *pacs) {
    long len = ::read(fd, buf, sizeof(buf));
    if (len <= 0) {
        return 0;
    }
    bool ret = decodeSlip(len, pacs);
    return (int) ret;
}

void serial::close() const {
    ::close(fd);
}

void serial::transfer(const uint8_t *color, size_t len) {
    if(len <= 0)return;
    uint8_t bufc[4096] = {};

    int cnt = 0;
    int i_buf = 0;
    while (true) {
        switch (color[cnt]) {
            case SLIP_END:
                bufc[i_buf] = (SLIP_ESC);
                i_buf++;
                bufc[i_buf] = (SLIP_ESC_END);
                i_buf++;
                break;
            case SLIP_ESC:
                bufc[i_buf] = (SLIP_ESC);
                i_buf++;
                bufc[i_buf] = (SLIP_ESC_ESC);
                i_buf++;
                break;
            default:
                bufc[i_buf] = (color[cnt]);
                i_buf++;
                break;
        }
        cnt++;
        if (cnt >= len) {
            bufc[i_buf] = (SLIP_END);
            i_buf++;
            break;
        }
    }

    ::write(fd, bufc, i_buf);

}

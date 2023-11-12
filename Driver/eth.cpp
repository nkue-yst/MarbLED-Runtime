//
// Created by chihiro on 23/11/01.
//

#include "eth.h"
#include <cmath>

Eth::Eth(const char *addr, uint16_t port, int mode, int ex_len){
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_port = htons(port);
    sock_addr.sin_addr.s_addr = inet_addr(addr);

    eth_mode = (mode == ETH_CONN_TCP) ? SOCK_STREAM : SOCK_DGRAM;
    expected_len = ex_len * 2;  // 16bit quantization
}

int Eth::tm_open(){
    sock_fd = socket(AF_INET, eth_mode, 0);
    if(sock_fd < 0){
        std::cerr << "socket open error" << std::endl;
        return -1;
    }

    std::cout << "socket open on port : " << htons(sock_addr.sin_port) << std::endl;

    if(eth_mode == SOCK_DGRAM) return 0;    // UDP does not require "connect"

    int ret = connect(sock_fd, (struct sockaddr *)&sock_addr, sizeof(struct sockaddr_in));
    if(ret < 0){
        std::cerr << "socket connect error" << std::endl;
    }

    return 0;
}

int Eth::read(std::vector<tm_packet> *pacs){
    if(eth_mode == SOCK_DGRAM) return -1; // Sensor data can only be received via TCP connection.

    pacs->clear();
    uint16_t data[1024] = {};
    ssize_t ret = recv(sock_fd, data, expected_len, 0);
    if(ret < expected_len){
        return -1;
    }

    for(unsigned int i = 0; i < expected_len; i++){
        tm_packet pac = {(uint16_t)i, data[i]};
        pacs->emplace_back(pac);
    }

    return (int)expected_len;
}

void Eth::transfer(const uint8_t *color, size_t len) {
    if(eth_mode == SOCK_STREAM) return; // Color Data can only be sent via UDP connection.

    uint32_t buf[2048] = {};
    uint8_t offset[3] = {24, 16, 8};
    int pix_cnt = int(len / 3);

    for(int i = 0; i < len; i++){
        int c = i % 3;
        int p = int(i / 3);

        buf[p] |= color[i] << offset[c];
    }

    ssize_t ret = sendto(sock_fd, buf, pix_cnt * 4, 0, (struct sockaddr *)&sock_addr, sizeof(sock_addr));
    if(ret != pix_cnt * 4)std::cerr << "send error" << std::endl;

}

void Eth::close() const{
    ::close(sock_fd);
}

Eth::~Eth(){
    ::close(sock_fd);
}
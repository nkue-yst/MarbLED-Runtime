//
// Created by chihiro on 23/11/01.
//

#ifndef MARBLED_RUNTIME_ETH_H
#define MARBLED_RUNTIME_ETH_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "bucket.h"

#define     ETH_CONN_UDP    0x01
#define     ETH_CONN_TCP    0x02

class Eth : public Bucket{
public:

private:
    int sock_fd{};
    struct sockaddr_in sock_addr{};

    int eth_mode;
    unsigned int expected_len = 0;

public:
    Eth(const char *addr, uint16_t port, int mode, int ex_len);
    int tm_open() override;
    int read(std::vector<tm_packet> *pacs) override;
    void transfer(const uint8_t *color, size_t len) override;
    void close() const override;
    ~Eth();
};


#endif //MARBLED_RUNTIME_ETH_H

/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * This defines an interface layer for talking to our network hub via
 * TCP. We can set an address, transmit messages to another node by
 * address, or receive messages.
 */

#ifndef _NETWORK_H
#define _NETWORK_H

#include <stdint.h>
#include <SDL.h>

class NetworkClient {
 public:
    void init(const char *host, const char *port);
    void exit(void);

    void setAddr(uint64_t addr);
    void tx(uint64_t addr, void *payload, int len);
    int rx(uint64_t *addr, uint8_t payload[256]);

 private:
    void disconnect();
    void txBytes(uint8_t *data, int len);
    void addrToBytes(uint64_t addr, uint8_t *bytes);
    uint64_t addrFromBytes(uint8_t *bytes);
    void setAddrInternal(uint64_t addr);
    void tryConnect();
    void rxIntoBuffer();
    static int threadFn(void *param);

    static const uint8_t NETHUB_SET_ADDR = 0x00;
    static const uint8_t NETHUB_MSG      = 0x01;
    static const uint8_t NETHUB_ACK      = 0x02;
    static const uint8_t NETHUB_NACK     = 0x03;

    struct addrinfo *addr;
    SDL_Thread *thread;
    SDL_sem *rx_sem;
    uint64_t rf_addr;
    int fd;
    int is_connected;
    int is_running;

    // Raw receive buffer
    int rx_count;
    uint8_t rx_buffer[1024];

    // Received packet buffer
    int rx_packet_len;
    uint64_t rx_addr;
    uint8_t rx_packet[256];
};


#endif

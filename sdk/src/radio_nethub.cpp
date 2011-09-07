/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Implementation of Radio based on the nethub daemon.
 *
 * This is only for simulation use. In this case, the SDK would be
 * compiled natively for the host machine that runs the cube emulator,
 * and it would connect directly to nethub via TCP.
 *
 * XXX: We aren't using threads or signals here (for portability),
 *      so we can only invoke ISR delegates from within halt().
 *      On real hardware, they can be invoked any time we aren't
 *      running another ISR.
 */

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   include <winsock2.h>
#   include <ws2tcpip.h>
#   pragma comment(lib, "ws2_32.lib")
#else
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <sys/select.h>
#   include <netinet/in.h>
#   include <fcntl.h>
#   include <netdb.h>
#   include <unistd.h>
#endif

#include "radio.h"

namespace Sifteo {

/*
 * Network protocol constants
 */
#define NETHUB_SET_ADDR 	0x00
#define NETHUB_MSG    		0x01
#define NETHUB_ACK		0x02
#define NETHUB_NACK		0x03

#define NETHUB_HDR_LEN		2
#define NETHUB_ADDR_LEN		8

#define TX_ACK_RESERVATION      (2 * 5)       // Room for five ACK packets
#define TX_MSG_RESERVATION	(32 + 8 + 2)  // Worst-case radio message

#define TX_PENDING_LIMIT	1	// Unacknowledged packet limit


/*
 * Private static data, doesn't need to be known outside this file.
 */
static struct NethubRadio {
    struct addrinfo *addr;
    fd_set rfds, wfds, efds;
    int fd;

    bool isConnected;
    uint8_t txPending;

    uint8_t txHead, txTail;   // txHead <= txTail
    uint8_t rxHead, rxTail;   // rxHead <= rxTail

    uint8_t txBuffer[192];
    uint8_t rxBuffer[192];
} self;


void Radio::open()
{
    /*
     * Initialize the radio device, and try to connect. We may not be
     * able to connect right away, though, or we may get
     * disconnected. We'll try to reconnect as necessary.
     */

    const char *nethub_host = "127.0.0.1";
    const char *nethub_port = "2405";

    struct addrinfo hints;
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    
    memset(&self, 0, sizeof self);
    self.fd = -1;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(nethub_host, nethub_port, &hints, &self.addr);
}

static void SifteoRadio_disconnect()
{
    if (self.fd >= 0) {
#ifdef _WIN32
        closesocket(self.fd);
#else
	close(self.fd);
#endif
    }

    self.fd = -1;
    self.isConnected = false;
}

#include <stdio.h>

static void SifteoRadio_tryConnect()
{
    if (self.fd < 0) {
	FD_ZERO(&self.rfds);
	FD_ZERO(&self.wfds);
	FD_ZERO(&self.efds);

	self.txPending = 0;
	self.rxHead = self.rxTail = 0;
	self.txHead = self.txTail = 0;

	self.fd = socket(self.addr->ai_family, self.addr->ai_socktype,
			 self.addr->ai_protocol);
	if (self.fd < 0)
	    return;
    }

    if (connect(self.fd, self.addr->ai_addr, self.addr->ai_addrlen)) {
	SifteoRadio_disconnect();
    } else {
	unsigned long arg;

	self.isConnected = true;

#ifdef _WIN32
	arg = 1;
	ioctlsocket(self.fd, FIONBIO, &arg);
#else
	fcntl(self.fd, F_SETFL, O_NONBLOCK | fcntl(self.fd, F_GETFL));
#endif

#ifdef SO_NOSIGPIPE
	arg = 1;
	setsockopt(self.fd, SOL_SOCKET, SO_NOSIGPIPE, &arg, sizeof arg);
#endif

#ifdef TCP_NODELAY
	arg = 1;
	setsockopt(self.fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&arg, sizeof arg);
#endif
    }
}

static void SifteoRadio_ack()
{
    /*
     * Enqueue a nethub ACK in our transmit buffer
     */

    struct AckPacket {
	uint8_t len;
	uint8_t msg;
    } *packet = (AckPacket *) &self.txBuffer[self.txTail];

    if (self.txTail + sizeof *packet <= sizeof self.txBuffer) {
	packet->len = 0;
	packet->msg = NETHUB_ACK;
	self.txTail += sizeof *packet;
    }
}

static void SifteoRadio_produce()
{
    /*
     * Enqueue a new radio message in our transmit buffer
     */

    struct MsgPacket {
	uint8_t len;
	uint8_t msg;
	uint8_t addr[5];       // Low bytes of 64-bit address
	uint8_t reserved[2];   // Unused address bytes
	uint8_t channel;       // High byte of 64-bit address
	uint8_t payload[1];
    } *packet = (MsgPacket *) &self.txBuffer[self.txTail];

    PacketTransmission tx;
    tx.packet.bytes = packet->payload;
    packet->msg = NETHUB_MSG;
    packet->reserved[0] = 0;
    packet->reserved[1] = 0;

    RadioManager::produce(tx);
    packet->len = tx.packet.len + NETHUB_ADDR_LEN;
    packet->channel = tx.dest->channel;
    memcpy(packet->addr, tx.dest->id, sizeof packet->addr);
    self.txTail += tx.packet.len + NETHUB_ADDR_LEN + NETHUB_HDR_LEN;
    self.txPending++;
}

static void SifteoRadio_consume()
{
    /*
     * Handle all of the packets we can handle from the receive buffer.
     */

    for (;;) {
	uint8_t queueLen = self.rxTail - self.rxHead;
	uint8_t *packet = &self.rxBuffer[self.rxHead];
	uint8_t packetLen = packet[0] + NETHUB_HDR_LEN;

	if (packetLen > queueLen)
	    break;

	switch (packet[1]) {

	case NETHUB_ACK:
	    self.txPending--;
	    break;

	case NETHUB_NACK:
	    self.txPending--;
	    RadioManager::timeout();
	    break;

	case NETHUB_MSG: {
	    SifteoRadio_ack();
	    if (packet[0] >= NETHUB_ADDR_LEN) {
		PacketBuffer pb;
		pb.bytes = &packet[NETHUB_ADDR_LEN + NETHUB_HDR_LEN];
		pb.len = packet[0] - NETHUB_ADDR_LEN;
		RadioManager::acknowledge(pb);
	    }
	    break;
	}
	}
	 
	self.rxHead += packetLen;
    }

    /*
     * Reclaim buffer space. Typically we'll be receiving whole
     * packets, so this isn't expected to be a frequent operation.
     */

    if (self.rxHead == self.rxTail) {
	self.rxHead = self.rxTail = 0;
    } else if (self.rxHead > sizeof self.rxBuffer / 2) {
	self.rxTail -= self.rxHead;
	memmove(self.rxBuffer, self.rxBuffer + self.rxHead, self.rxTail);
	self.rxHead = 0;
    }
}

static void SifteoRadio_recv()
{
    int len = recv(self.fd, self.rxBuffer + self.rxTail,
		   sizeof self.rxBuffer - self.rxTail, 0);
    if (len > 0)
	self.rxTail += len;
    else if (errno != EAGAIN)
	SifteoRadio_disconnect();
}

static void SifteoRadio_send()
{
    int ret = send(self.fd, self.txBuffer + self.txHead,
		   self.txTail - self.txHead, 0);
    if (ret > 0)
	self.txHead += ret;
    else if (errno != EAGAIN)
	SifteoRadio_disconnect();

    if (self.txHead == self.txTail)
	self.txHead = self.txTail = 0;
}

void Radio::halt()
{
    /*
     * In our implementation, halt() runs the select loop which
     * actually pumps data between our local buffers and a TCP socket.
     */

    if (!self.isConnected) {
	SifteoRadio_tryConnect();
	if (!self.isConnected) {
	    // Can't do anything until we get our nethub back. Sleep a bit and try again.
	    usleep(100000);
	    return;
	}
    }

    FD_SET(self.fd, &self.rfds);
    FD_SET(self.fd, &self.efds);

    if (self.txPending < TX_PENDING_LIMIT)
	FD_SET(self.fd, &self.wfds);
    else
	FD_CLR(self.fd, &self.wfds);

    int nfds = select(self.fd + 1, &self.rfds, &self.wfds, &self.efds, NULL);
    if (nfds <= 0)
	return;

    // Handle socket exceptions
    if (self.isConnected && FD_ISSET(self.fd, &self.efds))
	SifteoRadio_disconnect();

    // recv() -> rxBuffer
    if (self.isConnected && FD_ISSET(self.fd, &self.rfds) && self.rxTail < sizeof self.rxBuffer)
	SifteoRadio_recv();

    // Consume packets from rxBuffer, but only if we have room for ACKs
    if (self.isConnected && sizeof self.txBuffer - self.txTail >= TX_ACK_RESERVATION)
	SifteoRadio_consume();

    // Produce one packet at a time, if we have buffer space
    if (self.isConnected && self.txPending < TX_PENDING_LIMIT &&
	sizeof self.txBuffer - self.txTail >= TX_MSG_RESERVATION)
	SifteoRadio_produce();

    // txBuffer -> send()
    if (self.isConnected && self.txTail > self.txHead)
	SifteoRadio_send();
}

};  // namespace Sifteo

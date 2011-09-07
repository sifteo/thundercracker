/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Implementation of Sifteo::Radio based on the nethub daemon.
 *
 * This is only for simulation use. In this case, the SDK would be
 * compiled natively for the host machine that runs the cube emulator,
 * and it would connect directly to nethub via TCP.
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
#endif

#include "radio.h"

/*
 * Network protocol constants
 */
#define NETHUB_SET_ADDR 	0x00
#define NETHUB_MSG    		0x01
#define NETHUB_ACK		0x02
#define NETHUB_NACK		0x03
#define NETHUB_HDR_LEN		2
#define NETHUB_ADDR_LEN		8

/*
 * Private static data, doesn't need to be known outside this file.
 */
static struct NethubRadio {
    struct addrinfo *addr;
    Sifteo::Radio::InterruptModes irqModes, irqPending;
    bool isConnected;
    int fd;
    unsigned rxCount;
    uint8_t rxBuffer[128];
} self;


void Sifteo::Radio::open()
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
    
    if (getaddrinfo(nethub_host, nethub_port, &hints, &net.addr)) {
	perror("getaddrinfo");
	exit(1);
    }
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
    self.isConnected = 0;
}

static void SifteoRadio_tryConnect()
{
    if (self.fd < 0) {
	self.rxCount = 0;
	self.fd = socket(self.addr->ai_family, self.addr->ai_socktype,
			 self.addr->ai_protocol);
	if (self.fd < 0)
	    return;
    }

    if (!connect(self.fd, self.addr->ai_addr, self.addr->ai_addrlen)) {
	unsigned long arg;

	self.isConnected = true;

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

static void SifteoRadio_sendBytes(uint8_t *bytes, unsigned len)
{
    while (len > 0) {
	int ret = send(self.fd, bytes, len, 0);

	if (ret > 0) {
	    bytes += ret;
	    len -= ret;
	} else {
	    SifteoRadio_disconnect();
	    break;
	}
    }
}
	    
void Sifteo::Radio::send(const RadioAddress &addr, const PacketBuffer &buf)
{
    struct {
	uint8_t len;
	uint8_t msg;
	uint8_t addr[5];       // Low bytes of 64-bit address
	uint8_t reserved[2];   // Unused address bytes
	uint8_t channel;       // High byte of 64-bit address
	uint8_t payload[PacketBuffer::MAX_SIZE];
    } packet;

    if (!self.isConnected)
	SifteoRadio_tryConnect();
    if (!self.isConnected)
	return;

    packet.len = buf.len + NETHUB_ADDR_LEN;
    packet.msg = NETHUB_MSG;
    packet.reserved[0] = 0;
    packet.reserved[1] = 0;
    packet.channel = addr.channel;
    memcpy(packet.addr, addr.id, sizeof packet.addr);
    memcpy(packet.payload, buf.bytes, buf.len);

    SifteoRadio_sendBytes((uint8_t *)&packet, buf.len + NETHUB_HDR_LEN + NETHUB_ADDR_LEN);
}

void Sifteo::Radio::interruptSet(InterruptModes modes)
{
    self.irqModes = modes;
}

void Sifteo::Radio::recv(PacketBuffer &buf)
{
    /*
     * Pump our event loop until there's a message at the head of the buffer.
     * Note that for our purposes, a NACK counts as a timeout message.
     */

    while (self.rxCount < NETHUB_HDR_LEN ||
	   self.rxCount < NETHUB_HDR_LEN + self.rxBuffer[0] ||
	   (self.rxBuffer[1] != NETHUB_MSG && self.rxBuffer[1] != NETHUB_NACK))
	halt();

    if (self.rxBuffer[1] == NETHUB_MSG && self.rxBuffer[0] >= NETHUB_ADDR_LEN) {
	buf.len = self.rxBuffer[0] - NETHUB_ADDR_LEN;
	buf.timeout = false;
	memcpy(buf.bytes, self.rxBuffer + NETHUB_HDR_LEN + NETHUB_ADDR_LEN, buf.len);
    } else {
	buf.len = 0;
	buf.timeout = true;
    }

    {
	unsigned packetLen = self.rxBuffer[0] + NETHUB_HDR_LEN;
	self.rxCount -= packetLen;
	memmove(self.rxBuffer, self.rxBuffer + packetLen, self.rxCount);
    }
}

void Sifteo::Radio::halt()
{
}


static void network_rx_into_buffer(void)
{
    int recv_len = 1;
    fd_set rfds;

    FD_ZERO(&rfds);
	
    while (self.is_running && self.is_connected) {
	int packet_len = 2 + (int)self.rx_buffer[0];
	uint8_t packet_type = self.rx_buffer[1];

	if (self.rx_count >= packet_len) {
	    /*
	     * We have a packet! Is it a message? We currently ignore
	     * remote ACKs. (We're providing unidirectional flow
	     * control, not consuming the remote end's flow control.)
	     */
	    
	    if (packet_type == NETHUB_MSG) {
		static uint8_t ack[] = { 0, NETHUB_ACK };
		network_tx_bytes(ack, sizeof ack);

		// Make sure the packet buffer is available
		SDL_SemWait(self.rx_sem);

		self.rx_addr = network_addr_from_bytes(self.rx_buffer + 2);
		memcpy(self.rx_packet, self.rx_buffer + 10,
		       packet_len - 10);

		// This assignment transfers buffer ownership to the main thread
		self.rx_packet_len = packet_len - 10;
	    }

	    // Remove packet from buffer
	    memmove(self.rx_buffer, self.rx_buffer + packet_len, self.rx_count - packet_len);
	    self.rx_count -= packet_len;

	} else {
	    /*
	     * No packet buffered. Try to read more!
	     *
	     * We have the socket in nonblocking mode so that our
	     * writes will not block, but here we do want a blocking
	     * read.
	     */

	    FD_SET(self.fd, &rfds);
	    select(self.fd + 1, &rfds, NULL, NULL, NULL);
	    
	    recv_len = recv(self.fd, self.rx_buffer + self.rx_count,
			    sizeof self.rx_buffer - self.rx_count, 0);
	    if (recv_len <= 0) {
		if (recv_len == 0 || errno != EAGAIN)
		    network_disconnect();
		break;
	    }

	    self.rx_count += recv_len;
	}
    }
}

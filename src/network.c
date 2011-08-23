/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "network.h"

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#   include <winsock2.h>
#   include <ws2tcpip.h>
#   pragma comment(lib, "ws2_32.lib")
#else
#   include <sys/types.h>
#   include <sys/socket.h>
#   include <netinet/in.h>
#   include <fcntl.h>
#   include <netdb.h>
#endif

#define NETHUB_SET_ADDR 	0x00
#define NETHUB_MSG    		0x01
#define NETHUB_ACK		0x02
#define NETHUB_NACK		0x03

struct {
    struct addrinfo *addr;
    uint64_t rf_addr;
    int fd;
    int is_connected;
    int rx_count;
    uint8_t rx_buffer[1024];
} net;

static void network_set_nonblock(unsigned long mode)
{
#ifdef _WIN32
    ioctlsocket(net.fd, FIONBIO, &mode);
#else
    fcntl(net.fd, F_SETFL, mode ? O_NONBLOCK : 0);
#endif
}

static void network_try_connect(void)
{
    if (net.is_connected)
	return;

    if (net.fd < 0) {
	net.rx_count = 0;

	net.fd = socket(net.addr->ai_family, net.addr->ai_socktype, net.addr->ai_protocol);
	if (net.fd < 0)
	    return;

	network_set_nonblock(1);
    }

    if (!connect(net.fd, net.addr->ai_addr, net.addr->ai_addrlen)) {
	net.is_connected = 1;

	if (net.rf_addr)
	    network_set_addr(net.rf_addr);
    }
}

void network_init(const char *host, const char *port)
{
    struct addrinfo hints;
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    net.fd = -1;
    net.is_connected = 0;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    if (getaddrinfo(host, port, &hints, &net.addr)) {
	perror("getaddrinfo");
	exit(1);
    }
}

static void network_disconnect(void)
{
    if (net.fd >= 0)
	close(net.fd);
    net.fd = -1;
    net.is_connected = 0;
}   

void network_exit(void)
{
    network_disconnect();
    freeaddrinfo(net.addr);
}

static void network_tx_bytes(uint8_t *data, int len)
{
    while (len > 0) {
	int ret = send(net.fd, data, len, 0);

	if (ret > 0) {
	    len -= ret;
	    data += ret;
	} else {
	    network_disconnect();
	    break;
	}
    }
}

static void network_addr_to_bytes(uint64_t addr, uint8_t *bytes)
{
    int i;
    for (i = 0; i < 8; i++) {
	bytes[i] = addr;
	addr >>= 8;
    }
}

void network_set_addr(uint64_t addr)
{
    uint8_t packet[10] = { 0, NETHUB_SET_ADDR };
    network_addr_to_bytes(addr, &packet[2]);
    network_tx_bytes(packet, sizeof packet);
}

void network_tx(uint64_t addr, void *payload, int len)
{
    uint8_t buffer[512];

    network_try_connect();

    if (len <= 255 - 8) {
	buffer[0] = len + 8;
	buffer[1] = NETHUB_MSG;
	network_addr_to_bytes(addr, buffer + 2);
	memcpy(buffer + 10, payload, len);

	network_tx_bytes(buffer, len + 10);
    }
}

int network_rx(uint8_t payload[256])
{
    int recv_len = 1;
	
    for (;;) {
	int packet_len = 2 + (int)net.rx_buffer[0];
	uint8_t packet_type = net.rx_buffer[1];
	int result = 0;

	if (net.rx_count >= packet_len) {
	    /*
	     * We have a packet! Is it a message? We currently ignore
	     * remote ACKs. (We're providing unidirectionalflow
	     * control, not consuming the remote end's flow control.)
	     *
	     * We also ignore the source address, since our hardware
	     * doesn't know who sent a message.
	     */
	     
	    if (packet_type == NETHUB_MSG) {
		static uint8_t ack[] = { 0, NETHUB_ACK };
		network_tx_bytes(ack, sizeof ack);

		result = packet_len - 10;
		memcpy(payload, net.rx_buffer + 10, result);
	    }

	    // Remove packet from buffer
	    memmove(net.rx_buffer, net.rx_buffer + packet_len, net.rx_count - packet_len);
	    net.rx_count -= packet_len;

	    if (result)
		return result;

	} else {
	    /*
	     * No packet buffered. Try to read more!
	     */

	    network_try_connect();
	    recv_len = recv(net.fd, net.rx_buffer + net.rx_count,
			    sizeof net.rx_buffer - net.rx_count, 0);

	    if (recv_len <= 0) {
		if (recv_len == 0 || errno != EAGAIN)
		    network_disconnect();
		break;
	    }

	    net.rx_count += recv_len;
	}
    }

    return 0;
}


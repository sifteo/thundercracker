#!/usr/bin/env python
#
# Network hub for simulated radio environment
# M. Elizabeth Scott <beth@sifteo.com>
#
# Copyright <c> 2011 Sifteo, Inc. All rights reserved.
#

#
# This is a server which accepts incoming TCP connections from every
# other node in the system. This includes both transmitter and
# receiver nodes. Every node can specify its own address and channel
# at any time, and it can send a message to another node, specifying
# it by address and channel.
#
# Being the central entity that sees all this traffic, this daemon can
# also trace it all to stdout or log it, for debugging or analysis
# purposes.
#
# This daemon provides strict flow control, based on explicit ACK
# packets.  All consumers are required to ACK all packets they
# receive, and we collect and propagate those ACKs to the producer.
#
# This daemon does NOT implement any kind of realistic timing. All
# timing simulation is done by the individual nodes, so that it can be
# included in the same timing domain as the rest of the nodes'
# hardware simulation.  For example, the cubes can accept packets at a
# realistic rate according to their simulated CPU cycle counter, and a
# master node will then (due to the flow control) always write packets
# to that cube at a realistic rate.
#
# To be slightly hardware-agnostic, and really just for convenience's
# sake, we represent one radio "address" as an opaque 64-bit
# number. With the Nordic radios, this can include a packed
# representation of both the RF channel and the device address.
#
# The packet format is simple. Every packet has a two-byte header to
# identify message type and total length (not including the
# 2-byte header). There are only a few special packet types:
#
#  - Set my address:
#  
#     08 00 <uint64_t address>
#
#  - Packet data, to/from a specified address:
#
#     NN 01 <uint64_t address> <payload>
#
#  - Packet ACK. Sent in reply to a packet after it is fully delivered,
#    for flow control purposes. Clients must generate an ACK after every
#    packet they receive.
#
#     00 02
#
#  - Packet NACK. Sent in place of an ACK if no clients were listening
#
#     00 03
#

import argparse, asyncore, socket, struct, binascii, time


def log(origin, message):
    print "%s -- %s" % (origin, message)

def main():
    parser = argparse.ArgumentParser(description="Network hub for Sifteo radio simulation")
    parser.add_argument('--port', metavar='N', type=int, default=2405,
                        help="TCP port number to listen on")
    parser.add_argument('--bind', metavar='ADDR', default="",
                        help="Network interface to bind our server to")

    args = parser.parse_args()
    server = NethubServer(args.bind, args.port)
    asyncore.loop()

def test_producer(addr):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(('127.0.0.1', 2405))
    s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    i = 0
    while True:
        i += 1
        print "Sending %d" % i
        s.send(struct.pack("<BBQI", 12, 1, addr, i))
        s.recv(8192)

def test_consumer(addr):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(('127.0.0.1', 2405))
    s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 512)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 512)
    s.send(struct.pack("<BBQ", 8, 0, addr))
    while True:
        print binascii.b2a_hex(s.recv(14))
        time.sleep(1)
        s.send("\x00\x02")


class Hub:
    def __init__(self):
        self.addrToClients = {}
        self.clientToAddr = {}

    def removeClient(self, client):
        if client in self.clientToAddr:
            addr = self.clientToAddr[client]
            del self.clientToAddr[client]
            l = self.addrToClients[addr]
            l.remove(client)
            if not l:
                del self.addrToClients[addr]

    def clientSetAddr(self, client, addr):
        self.removeClient(client)        
        self.clientToAddr[client] = addr
        self.addrToClients.setdefault(addr, []).append(client)
    
    def getDests(self, addr):
        return self.addrToClients.get(addr, ())


class NethubClient(asyncore.dispatcher_with_send):
    def __init__(self, hub, addr, s):
        self.hub = hub
        self.address = 0xFFFFFFFFFFFFFFFF & hash(addr)  # Assign a temporary address
        self.recv_buffer = ""

        # Currently pending output message
        self.current_dests = ()
        self.current_msg = None
        
        # Waiting on ACKs for a message that was sent to this client?
        self.acks_pending = 0

        log(self, "New connection from %s:%d" % addr)

        s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        asyncore.dispatcher_with_send.__init__(self, s)

    def __str__(self):
        return "client(%016x)" % self.address

    def handle_close(self):
        self.close()

    def close(self):
        log(self, "Closed connection")
        self.acks_pending = 0
        self.hub.removeClient(self.address)
        asyncore.dispatcher_with_send.close(self)

    def handle_read(self):
        self.recv_buffer += self.recv(8192)
        while self.recv_buffer:
            packetLen = ord(self.recv_buffer[0]) + 2
            if len(self.recv_buffer) >= packetLen and self.readable():
                # We can handle one message
                self.handle_packet(self.recv_buffer[:packetLen])
                self.recv_buffer = self.recv_buffer[packetLen:]
            else:
                # Waiting
            	return

    def handle_packet(self, packet):
        # Try to handle the next incoming packet. Returns True if we
        # can consume it, or False if not. This should only return
        # False if the client is (or is about to become) non-readable.

        ptype = ord(packet[1])
        f = getattr(self, "packet_%02x" % ptype, None)
        if f:
            return f(packet)
        else:
            log(self, "Unhandled packet %s" % binascii.b2a_hex(packet))

    def packet_02(self, packet):
        if len(packet) != 2:
            log(self, "Incorrect length for ACK")
        if self.acks_pending:
            self.acks_pending -= 1
        else:
            log(self, "Received unsolicited ACK")

    def packet_00(self, packet):
        if len(packet) != 10:
            log(self, "Incorrect length for address packet")
        else:
            addr = struct.unpack("<Q", packet[2:])
            log(self, "Setting address to %016x" % addr)
            self.address = addr
            self.hub.clientSetAddr(self, self.address)

    def packet_01(self, packet):
        if len(packet) < 8:
            log(self, "Incorrect length for data packet")
            return

        # Dispatch one message at a time
        assert not self.current_dests

        addr = struct.unpack("<Q", packet[2:10])
        payload = packet[10:]
        self.current_dests = list(self.hub.getDests(addr))

        if self.current_dests:
            self.current_msg = payload
        else:
            # Nobody's listening, send a NACK
            log(self, "NAK, nobody is listening at %016x" % addr)
            self.send('\x00\x03')

    def readable(self):
        # Process our pending message, if any, and see if we're ready to accept more data
        if self.current_dests:
            still_busy = []
            for dest in self.current_dests:

                if not dest.connected:
                    # Client was closed, ignore it and pretend the message was delivered.
                    pass
                elif dest.acks_pending:
                    # Keep waiting on this client
                    still_busy.append(dest)
                else:
                    # Send it, and expect an ACK
                    log(self, "send to %s -- %s" % (dest, binascii.b2a_hex(self.current_msg)))
                    dest.acks_pending += 1
                    dest.send(struct.pack("<BBQ", 8+len(self.current_msg), 1,
                                          self.address) + self.current_msg)

            self.current_dests = still_busy
            if not still_busy:
                # Fully delivered. Send our own ACK.
                self.send('\x00\x02')

        return not self.current_dests


class NethubServer(asyncore.dispatcher):
    def __init__(self, host, port):
        asyncore.dispatcher.__init__(self)
        self.hub = Hub()

        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.set_reuse_addr()
        self.bind((host, port))
        self.listen(5)
        log(self, "Listening at %s:%s" % (host, port))

    def __str__(self):
        return "server"

    def handle_accept(self):
        pair = self.accept()
        if pair is None:
            pass
        else:
            sock, addr = pair
            client = NethubClient(self.hub, addr, sock)


if __name__ == "__main__":
    main()

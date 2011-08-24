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

import argparse, asyncore, socket, struct
import binascii, time, sys, threading

class Logger:
    verbose = True
    def __call__(self, origin, message):
        if self.verbose:
            sys.stderr.write("%s -- %s\n" % (origin, message))
log = Logger()

def hexint(i):
    return int(i, 16)

def main():
    parser = argparse.ArgumentParser(description="Network hub for Sifteo radio simulation")
    parser.add_argument('-q', '--quiet', action='store_true',
                        help="Don't log to stderr")
    parser.add_argument('-p', '--port', metavar='N', type=int, default=2405,
                        help="TCP port number to listen/connect on")
    parser.add_argument('--bind', metavar='ADDR', default="",
                        help="Network interface to bind our server to")
    
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--server', action='store_true', default=True,
                       help="Run the central network hub server")
    group.add_argument('--pipe', metavar='ADDR', type=hexint,
                       help="stdin transmits to ADDR, responses go to stdout")
    group.add_argument('--produce', metavar='ADDR', type=hexint,
                       help="spam junk packets at ADDR")
    group.add_argument('--consume', metavar='ADDR', type=hexint,
                       help="listen on ADDR, print incoming packets to stdout")

    args = parser.parse_args()
    log.verbose = not args.quiet

    if args.pipe:
        client = ClientPipe(args.bind, args.port, args.pipe)
    elif args.produce:
        client = ClientProducer(args.bind, args.port, args.produce)
    elif args.consume:
        client = ClientConsumer(args.bind, args.port, args.consume)
    elif args.server:
        server = NethubServer(args.bind, args.port)

    asyncore.loop()


class PacketDispatcher(asyncore.dispatcher_with_send):
    """Base dispatcher class which handles incoming packets, buffering and dispatching
       each one to a member function.
       """
    def __init__(self, s=None):
        self.recv_buffer = ""
        asyncore.dispatcher_with_send.__init__(self, s)

    def handle_read(self):
        self.recv_buffer += self.recv(8192)
        while self.recv_buffer:
            packetLen = ord(self.recv_buffer[0]) + 2
            if len(self.recv_buffer) >= packetLen:
                # We can handle one message
                self.handle_packet(self.recv_buffer[:packetLen])
                self.recv_buffer = self.recv_buffer[packetLen:]
            else:
                # Waiting
            	return

    def handle_packet(self, packet):
        ptype = ord(packet[1])
        f = getattr(self, "packet_%02x" % ptype, None)
        if f:
            f(packet)
        else:
            log(self, "Unhandled packet %s" % binascii.b2a_hex(packet))

    def packet_01(self, packet):
        if len(packet) < 8:
            log(self, "Incorrect length for data packet")
            return
        addr = struct.unpack("<Q", packet[2:10])[0]
        payload = packet[10:]
        self.handle_msg(addr, payload)

    def ack(self):
        self.send("\x00\x02")

    def handle_msg(self, addr, payload):
        self.ack()


class AbstractClient(PacketDispatcher):
    def __init__(self, host, port):
        PacketDispatcher.__init__(self)
        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.connect((host, port))
        self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

    def __str__(self):
        return "client"

    def setAddress(self, addr):
        self.send(struct.pack("<BBQ", 8, 0, addr))


class ClientProducer(AbstractClient):
    def __init__(self, host, port, destAddr):
        self.destAddr = destAddr
        self.sequence = 0
        AbstractClient.__init__(self, host, port)
        self.produce()

    def produce(self):
        self.sequence = (self.sequence + 1) & 0xFFFFFFFF
        log(self, "sending to %016x, %08x" % (self.destAddr, self.sequence))
        self.send(struct.pack("<BBQI", 12, 1, self.destAddr, self.sequence))

    def packet_02(self, p):
        # ACK
        self.produce()

    def packet_03(self, p):
        # NACK
        self.produce()


class ClientConsumer(AbstractClient):
    def __init__(self, host, port, srcAddr):
        AbstractClient.__init__(self, host, port)
        self.setAddress(srcAddr)

    def handle_msg(self, addr, payload):
        self.ack()
        log(self, "received from %016x, %s" % (addr, binascii.b2a_hex(payload)))
        time.sleep(0.2)

    def packet_02(self, p):
        # ACK
        pass

    def packet_03(self, p):
        # NACK
        pass


class ClientPipe(AbstractClient):
    def __init__(self, host, port, destAddr):
        self.destAddr = destAddr
        AbstractClient.__init__(self, host, port)

        self.sem = threading.Semaphore(0)
        self.thread = threading.Thread(target=self.thread_func)
        self.thread.daemon = True
        self.thread.start()

    def handle_msg(self, addr, payload):
        self.ack()
        if addr == self.destAddr:
            sys.stdout.write("%s\n" % binascii.b2a_hex(payload))
        else:
            log(self, "Unsolicited packet from %016x" % addr)

    def thread_func(self):
        while True:
            l = sys.stdin.readline().strip()
            try:
                payload = binascii.a2b_hex(l)
            except TypeError:
                log(self, "Badly formed input line, packets must be in hexadecimal")
                continue

            self.send(struct.pack("<BBQ", 8+len(payload), 1,
                                  self.destAddr) + payload)
            self.sem.acquire()

    def packet_02(self, p):
        # ACK
        self.sem.release()

    def packet_03(self, p):
        # NACK
        self.sem.release()


class Hub:
    def __init__(self):
        self.addrToClients = {}
        self.clientToAddr = {}

    def removeClient(self, client):
        if id(client) in self.clientToAddr:
            addr = self.clientToAddr[id(client)]
            del self.clientToAddr[id(client)]
            l = self.addrToClients[addr]
            l.remove(client)
            if not l:
                del self.addrToClients[addr]

    def clientSetAddr(self, client, addr):
        self.removeClient(client)        
        self.clientToAddr[id(client)] = addr
        self.addrToClients.setdefault(addr, []).append(client)
    
    def getDests(self, addr):
        return [l for l in self.addrToClients.get(addr, ()) if l.connected]


class NethubClient(PacketDispatcher):
    # How many packets can we send out without expecting an ACK?
    MAX_TX_DEPTH = 4

    def __init__(self, hub, addr, s):
        self.hub = hub
        self.recv_buffer = ""

        # Messages that we haven't started dispatching yet
        self.msg_backlog = []

        # Currently pending output message
        self.current_dests = ()
        self.current_msg = None
        
        # Current TX queue depth for this node
        self.tx_depth = 0

        # Assign a temporary address
        self.setAddress(hash(addr))

        log(self, "New connection from %s:%d" % addr)

        s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        PacketDispatcher.__init__(self, s)

    def __str__(self):
        return "client(%016x)" % self.address

    def setAddress(self, addr):
        self.address = 0xFFFFFFFFFFFFFFFF & addr
        self.hub.clientSetAddr(self, self.address)

    def handle_close(self):
        self.close()

    def close(self):
        log(self, "Closed connection")
        self.tx_depth = 0
        self.hub.removeClient(self.address)
        PacketDispatcher.close(self)

    def packet_02(self, packet):
        if len(packet) != 2:
            log(self, "Incorrect length for ACK")
        if self.tx_depth:
            self.tx_depth -= 1
        else:
            log(self, "Received unsolicited ACK")

    def packet_00(self, packet):
        if len(packet) != 10:
            log(self, "Incorrect length for address packet")
        else:
            addr = struct.unpack("<Q", packet[2:])[0]
            log(self, "Setting address to %016x" % addr)
            self.setAddress(addr)

    def handle_msg(self, addr, payload):
        self.msg_backlog.append((addr, payload))
        self.handle_msg_backlog()

    def handle_msg_backlog(self):
        while True:
            # Dispatch one message at a time
            if not self.current_dests:
                if not self.msg_backlog:
                    return
                self.prepare_next_dest()

            # Process our pending message, if any
            if self.current_dests:
                if not self.process_current_dest():
                    return

    def prepare_next_dest(self):
        addr, payload = self.msg_backlog[0]
        self.msg_backlog = self.msg_backlog[1:]

        self.current_dests = list(self.hub.getDests(addr))
        if self.current_dests:
            self.current_msg = payload
        else:
            # Nobody's listening, send a NACK
            log(self, "NAK, nobody is listening at %016x" % addr)
            self.send('\x00\x03')

    def process_current_dest(self):
        # Try to further dispatch the current packet. Returns True on completion

        still_busy = []
        for dest in self.current_dests:

            if not dest.connected:
                # Disconnected while we were waiting, ignore it
                pass
            elif dest.tx_depth > self.MAX_TX_DEPTH:
                # Keep waiting on this client
                still_busy.append(dest)
            else:
                # Send it, and expect an ACK
                log(self, "send to %s -- %s" % (dest, binascii.b2a_hex(self.current_msg)))
                dest.tx_depth += 1
                dest.send(struct.pack("<BBQ", 8+len(self.current_msg), 1,
                                      self.address) + self.current_msg)

        self.current_dests = still_busy
        if not still_busy:
            # Fully delivered. Send our own ACK.
            self.ack()
            return True

    def readable(self):
        self.handle_msg_backlog()
        return PacketDispatcher.readable(self)


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

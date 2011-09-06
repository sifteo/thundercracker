#!/usr/bin/env python
#
# Simple flash loadstream sender.
# M. Elizabeth Scott <beth@sifteo.com>
#
# Copyright <c> 2011 Sifteo, Inc. All rights reserved.
#

import argparse, socket, asyncore, threading
import Queue, struct, time, sys

def main():
    parser = argparse.ArgumentParser(description="Simple flash loadstream sender")
    parser.add_argument('file', help="Binary loadstream filename")
    args = parser.parse_args()

    net = NetworkMaster()
    loader = FlashLoader(net)
    data = open(args.file, 'rb').read()
    loader.send(data)


class FlashLoader:
    """Send a flash loadstream, with flow control, as fast as the cube will accept it."""

    OP_FLASH_QUEUE = 0xC0
    ACK_FIFO_BYTES = 11
    PACKET_SIZE = 32
    THRESHOLD = 16
    FIFO_MAX = 63

    def __init__(self, net):
        self.net = net
        self.cv = threading.Condition()
        self.buffer_space = self.FIFO_MAX
        self.prev_ack_byte = None
        net.register_callback(self._callback)

    def send(self, data):
        while data:
            self.cv.acquire()

            while self.buffer_space < self.THRESHOLD or self.prev_ack_byte is None: 
                self.cv.wait(1)

            blockSize = min(len(data), self.buffer_space)
            self.buffer_space -= blockSize
            self.cv.release()

            print "Sending %d bytes" % blockSize
            self._sendBlock(chr(self.OP_FLASH_QUEUE + blockSize - 1) + data[:blockSize])
            data = data[blockSize:]

    def _sendBlock(self, data):
        while data:
            packetSize = min(len(data), self.PACKET_SIZE)
            self.net.send_msg(data[:packetSize])
            data = data[packetSize:]

    def _callback(self, ack):
        ack_byte = ord(ack[self.ACK_FIFO_BYTES])

        self.cv.acquire()

        if self.prev_ack_byte is not None:
            ack_len = 0xFF & (ack_byte - self.prev_ack_byte)
            if ack_len:
                self.buffer_space += ack_len
                self.cv.notify()

        self.prev_ack_byte = ack_byte
        self.cv.release()


class NetworkMaster(asyncore.dispatcher_with_send):
    """Simple radio interface emulation, using nethub.

       - Incoming telemetry packets simply update the 'latest' telemetry state,
         and optionally invoke any registered callbacks.

       - Outgoing packets are sent as fast as possible. If we have any 'dead air'
         in between packets, we'll send a 'ping' just to get a fresh telemetry
         update in response.

       """    
    LOCAL_TX_DEPTH = 5 		# Depth of local transmit queue
    REMOTE_TX_DEPTH = 2		# Number of unacknowledged transmitted packets in flight

    def __init__(self, address=0x020000e7e7e7e7e7, host="127.0.0.1", port=2405):
        asyncore.dispatcher_with_send.__init__(self, map={})

        self.recv_buffer = ""
        self.tx_queue = Queue.Queue(self.LOCAL_TX_DEPTH)
        self.tx_depth = 0
        self.ping_buffer = struct.pack("<BBQ", 8, 1, address)
        
        self.callbacks = []
        self.telemetry = [0] * 32
        self.address = address

        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
        self.connect((host, port))
        self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

        self.thread = threading.Thread(target=self._threadMain)
        self.thread.daemon = True
        self.thread.start()

    def _threadMain(self):
        self._pump_tx_queue()
        asyncore.loop(map=self._map)

    def _pump_tx_queue(self):
        if self.tx_depth < self.REMOTE_TX_DEPTH:
            self.tx_depth += 1
            try:
                self.send(self.tx_queue.get(block=False))
            except Queue.Empty:
                self.send(self.ping_buffer)

    def register_callback(self, cb):
        self.callbacks.append(cb)

    def send_msg(self, payload):
        data = struct.pack("<BBQ", 8+len(payload), 1, self.address) + payload
        while True:
            try:
                self.tx_queue.put(data, timeout=1)
                return
            except Queue.Full:
                # Try again (Don't block for too long, or the process will be hard to kill)
                pass

    def handle_read(self):
        self.recv_buffer += self.recv(8192)
        while self.recv_buffer:
            packetLen = ord(self.recv_buffer[0]) + 2
            if len(self.recv_buffer) >= packetLen:
                # We can handle one message
                self._handle_packet(self.recv_buffer[:packetLen])
                self.recv_buffer = self.recv_buffer[packetLen:]
            else:
                # Waiting
            	return

    def _handle_packet(self, packet):
        msg_type = packet[1]

        if msg_type == '\x01':
            # ACK
            self.send("\x00\x02")

            # Store telemetry updates, and immediately notify callbacks
            if len(packet) > 10:
                self.telemetry = packet[10:]
                for cb in self.callbacks:
                    cb(self.telemetry)

        elif msg_type == '\x02':
            # ACK
            self.tx_depth -= 1
            self._pump_tx_queue()

        elif msg_type == '\x03':
            # NAK. Handle this like an ACK too, but delay
            # before sending anything else, so we don't spin
            # eating CPU if there's no destination to rate-limit
            # our transmissions.
            time.sleep(0.1)
            self.tx_depth -= 1
            self._pump_tx_queue()


if __name__ == "__main__":
    main()

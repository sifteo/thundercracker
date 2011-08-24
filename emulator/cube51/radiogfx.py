#!/usr/bin/env python

import socket, threading, struct


def main():
    tr = TileRenderer(ThreadedNethubInterface(), 0x98000)
    m = Map("assets/earthbound_fourside_full.map", width=256)

    while True:
        for i in range(256<<3):
            tr.pan(i&7, 0)
            tr.blit(m, srcx=i>>3, srcy=50, w=20, h=20)
            tr.refresh()


class Map:
    def __init__(self, filename=None, width=None):
        self.width = width
        if filename:
            self.loadFromFile(filename)

    def loadFromFile(self, filename):
        self.loadFromString(open(filename, "rb").read())

    def loadFromString(self, s):
        self.loadFromArray(struct.unpack("<%dH" % (len(s) / 2), s))

    def loadFromArray(self, data):
        self.data = data
        self.height = len(data) // self.width

    
class TileRenderer:
    def __init__(self, net, baseAddr):
        self.net = net
        self.baseAddr = baseAddr
        self.vram = [0] * 1024
        self.dirty = {}
        self.fill(0)

    def refresh(self):
        for chunk in self.dirty:
            bytes = [chunk] + self.vram[chunk * 31:(chunk+1) * 31]
            self.net.send(''.join(map(chr, bytes)))
        self.dirty = {}    

    def pan(self, x, y):
        """Set the hardware panning registers"""
        self.vram[0] = x
        self.vram[1] = y
        self.dirty[0] = True

    def plot(self, tile, x, y):
        """Draw a tile in the hardware tilemap, by coordinate and tile index."""
        addr = self.baseAddr + (tile << 7)
        offset = 0x001F + x*2 + y*40
        self.vram[offset] = (addr >> 6) & 0xFE
        self.vram[offset+1] = (addr >> 13) & 0xFE
        self.dirty[offset // 31] = True

    def fill(self, tile, x=0, y=0, w=20, h=20):
        """Fill a box of tiles, all using the same index"""
        for i in range(w):
            for j in range(h):
                self.plot(tile, x+i, y+j)

    def blit(self, mapobj, dstx=0, dsty=0, srcx=0, srcy=0, w=None, h=None):
        """Copy a rectangular block of tile indices from a map to the hardware buffer"""
        w = w or mapobj.width
        h = h or mapobj.height
        for j in xrange(h):
            for i in xrange(w):
                self.plot(mapobj.data[(srcx+i) + mapobj.width * (srcy+j)], dstx+i, dsty+j)


class ThreadedNethubInterface:
    def __init__(self, callback=None, address=0x020000e7e7e7e7e7, host="127.0.0.1", port=2405):
        self.callback = callback
        self.address = address
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect((host, port))
        self.socket.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

        # Semaphore controls TX queue depth
        self.tx_depth = 5
        self.ack_sem = threading.Semaphore(self.tx_depth)

        self.thread = threading.Thread(target=self.rx_thread)
        self.thread.daemon = True
        self.thread.start()

    def send(self, payload):
        self.ack_sem.acquire()
        self.socket.send(struct.pack("<BBQ", 8+len(payload), 1, self.address) + payload)

    def rx_thread(self):
        try:
            buf = ""
            while True:
                if len(buf) >= 2 and len(buf) >= ord(buf[0]) + 2:
                    packet_len = ord(buf[0]) + 2
                    msg_type = buf[1]

                    if msg_type == '\x01':
                        self.socket.send('\x00\x02')
                        if self.callback:
                            self.callback(buf[2:])

                    elif msg_type in '\x02\x03':
                        self.ack_sem.release()

                    buf = buf[packet_len:]
                else:
                    block = self.socket.recv(8192)
                    if not block:
                        raise IOError("Socket receive error")
                    buf += block
        finally:
            for i in range(self.tx_depth):
                self.ack_sem.release()


if __name__ == "__main__":
    main()

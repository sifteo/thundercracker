#!/usr/bin/env python

import socket, threading, struct, time, math


def main():
    tr = TileRenderer(ThreadedNethubInterface(), 0x98000)
    m = Map("assets/earthbound_fourside_full.map", width=256)
    ms = MapScroller(tr, m)

    while True:
        t = time.clock() * 2.5
        ms.scroll(int(512 + 150 * math.sin(t)),
                  int(512 + 150 * math.cos(t)))
        tr.refresh()


class MapScroller:
    """Implements efficient tear-free scrolling, by only updating
       tiles that are off-screen, just before they come on-screen.
       """

    def __init__(self, renderer, mapobj):
        self.tr = renderer
        self.map = mapobj

    def scroll(self, x, y):
        # Scroll to the specified coordinates. We could be WAY smarter about
        # the actual process of doing this update, but right now I'm implementing
        # it in a totally brute-force manner to keep this test harness simple.
        #
        # This method works because 'blit' knows how to wrap around on the VRAM
        # buffer, and because our TileRenderer is good at removing unnecessary changes.

        self.tr.pan(x % 160, y % 160)
        self.tr.blit(self.map, dstx=x>>3, dsty=y>>3,
                     srcx=x>>3, srcy=y>>3, w=17, h=17)


class Map:
    """Container for tile-based map/index data."""

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
    """Abstraction for the tile-based graphics renderer. Keeps an in-memory
       copy of the cube's VRAM, and sends updates via the radio as necessary.
       """

    FRAME_COUNT = 10
    MAX_FRAMES_AHEAD = 2
    
    def __init__(self, net, baseAddr):
        self.net = net
        self.baseAddr = baseAddr
        self.vram = [0] * 1024
        self.dirty = {}

        # Flow control
        self.local_frame_count = 0
        self.remote_frame_count = 0
        self.net.registerCallback(self._telemetryCb)

    def _telemetryCb(self, t):
        self.remote_frame_count = t[self.FRAME_COUNT]

    def refresh(self):
        # Flow control
        self.local_frame_count = (self.local_frame_count + 1) & 0xFF
        while ((0xFF & (self.local_frame_count - self.remote_frame_count))
               > self.MAX_FRAMES_AHEAD):
            # XXX: Waste time in a less ugly manner with variable payload len
            self.net.send('\x20')
            
        # Trigger the firmware to refresh the LCD 
        self.poke(802, self.local_frame_count)

        chunks = self.dirty.keys()
        chunks.sort()
        self.dirty = {}    
 
        for chunk in chunks:
            bytes = [chunk] + self.vram[chunk * 31:(chunk+1) * 31]
            self.net.send(''.join(map(chr, bytes)))

    def poke(self, addr, value):
        if self.vram[addr] != value:
            self.vram[addr] = value
            self.dirty[addr // 31] = True

    def pan(self, x, y):
        """Set the hardware panning registers"""
        self.poke(800, x)
        self.poke(801, y)

    def plot(self, tile, x, y):
        """Draw a tile in the hardware tilemap, by coordinate and tile index."""
        addr = self.baseAddr + (tile << 7)
        offset = (x % 20)*2 + (y % 20)*40
        self.poke(offset, (addr >> 6) & 0xFE)
        self.poke(offset + 1, (addr >> 13) & 0xFE)

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
    """Simple radio interface emulation, using nethub. This can send
       packets synchronously, and invoke a callback asynchronously
       when any responses are received from the cube.
       """

    def __init__(self, address=0x020000e7e7e7e7e7, host="127.0.0.1", port=2405):
        self.callbacks = []
        self.telemetry = [0] * 32
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

    def registerCallback(self, cb):
        self.callbacks.append(cb)

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

                        # Store telemetry updates, and immediately notify callbacks
                        if packet_len > 10:
                            self.telemetry = map(ord, buf[10:packet_len])
                            for cb in self.callbacks:
                                cb(self.telemetry)

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

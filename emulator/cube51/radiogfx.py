#!/usr/bin/env python

import socket, asyncore, threading, Queue, struct, time, math


def main():
    tr = TileRenderer(NetworkMaster(), 0x98000)
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

    # Telemetry packet offset
    FRAME_COUNT = 10

    MAX_FRAMES_AHEAD = 3
    WARP_FRAMES_AHEAD = 10
    
    def __init__(self, net, baseAddr):
        self.net = net
        self.baseAddr = baseAddr
        self.vram = [0] * 1024
        self.dirty = {}

        # Flow control
        self.local_frame_count = 0
        self.remote_frame_count = 0
        self.net.register_callback(self._telemetryCb)

    def _telemetryCb(self, t):
        self.remote_frame_count = t[self.FRAME_COUNT]

    def refresh(self):
        # Flow control. We aren't staying strictly synchronous with
        # the cube firmware here, since that would leave the cube idle
        # when it could be rendering. But we'll keep a coarser-grained count of
        # how much faster we are than the cube, and slow down if needbe.

        self.local_frame_count = (self.local_frame_count + 1) & 0xFF
        while True:
            ahead = 0xFF & (self.local_frame_count - self.remote_frame_count)

            if ahead <= self.MAX_FRAMES_AHEAD:
                break
            elif ahead > self.WARP_FRAMES_AHEAD:
                # Way behind! Let's do a time warp...
                self.local_frame_count = self.remote_frame_count
            else:
                # XXX: Ugly way to kill time...
                time.sleep(0)
            
        # Trigger the firmware to refresh the LCD 
        self.poke(802, self.local_frame_count)

        chunks = self.dirty.keys()
        chunks.sort()
        self.dirty = {}    
 
        for chunk in chunks:
            bytes = [chunk] + self.vram[chunk * 31:(chunk+1) * 31]
            self.net.send_msg(''.join(map(chr, bytes)))

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


class NetworkMaster(asyncore.dispatcher_with_send):
    """Simple radio interface emulation, using nethub.

       - Incoming telemetry packets simply update the 'latest' telemetry state,
         and optionally invoke any registered callbacks.

       - Outgoing packets are sent as fast as possible. If we have any 'dead air'
         in between packets, we'll send a 'ping' just to get a fresh telemetry
         update in response.

       """    
    LOCAL_TX_DEPTH = 2 		# Depth of local transmit queue
    REMOTE_TX_DEPTH = 2		# Number of unacknowledged transmitted packets in flight

    def __init__(self, address=0x020000e7e7e7e7e7, host="127.0.0.1", port=2405):
        asyncore.dispatcher_with_send.__init__(self, map={})

        self.recv_buffer = ""
        self.tx_queue = Queue.Queue(self.LOCAL_TX_DEPTH)
        self.tx_depth = 0
        self.ping_buffer = struct.pack("<BBQB", 9, 1, address, 0x20)
        
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
                self.telemetry = map(ord, packet[10:])
                for cb in self.callbacks:
                    cb(self.telemetry)

        elif msg_type in '\x02\x03':
            # Incoming ACK or NACK
            self.tx_depth -= 1
            self._pump_tx_queue()


if __name__ == "__main__":
    main()

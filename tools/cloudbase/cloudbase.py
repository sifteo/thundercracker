#!/usr/bin/env python

# to install elftools: `pip install pyelftools`

import sys, elftools, socket, argparse
from elftools.elf.elffile import ELFFile

# Installation Opcodes
OP_WRITE_HEADER = 0
OP_PAYLOAD      = 1
OP_COMMIT       = 2

# SLIP Codes
END             = 0300  # indicates end of packet
ESC             = 0333  # indicates byte stuffing
ESC_END         = 0334  # ESC ESC_END means END data byte
ESC_ESC         = 0335  # ESC ESC_ESC means ESC data byte

# largest packet the base can handle
MAX_PACKET      = 64


class SlipPacket:
    def __init__(self, op):
        self.data = []
        self.append(op)

    def append(self, b):
        if b == END:
            self.data.append(ESC)
            self.data.append(ESC_END)
        elif b == ESC:
            self.data.append(ESC)
            self.data.append(ESC_ESC)
        else:
            self.data.append(b)

        return self

    def end(self):
        self.data.append(END)

    def length(self):
        return len(self.data)


class Installer:
    def __init__(self, host, port):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # self.sock.connect(host, port)

    def writeHeader(self, sz, package):
        pkt = SlipPacket(OP_WRITE_HEADER)
        pkt.append(sz & 0xff).append((sz >> 8) & 0xff).append((sz >> 16) & 0xff).append((sz >> 24) & 0xff)
        for c in package:
            pkt.append(c)
        return self.send(pkt)

    def writeFileContents(self, sz, filepath):
        with open(filepath, 'rb') as f:
            while sz > 0:
                chunksz = min(sz, MAX_PACKET - 2)
                pkt = SlipPacket(OP_PAYLOAD)

                while pkt.length() < chunksz:
                    pkt.append(f.read(1))
                self.send(pkt)
                sz -= pkt.length()

    def commit(self):
        pkt = SlipPacket(OP_COMMIT)
        return self.send(pkt)

    def send(self, pkt):
        """
        Send this packet and wait for a response byte
        """
        pkt.end()
        # print pkt.data
        if pkt.length() > MAX_PACKET:
            raise ValueError("packet too large")
        return 0
        # self.sock.send(pkt.data)
        # return self.sock.recv(1)


def installableELFSize(filepath):
    with open(filepath, 'rb') as f:
        elffile = ELFFile(f)
        sz = 0

        for segment in elffile.iter_segments():
            sz = max(sz, segment['p_offset'] + segment['p_filesz'])
        return sz


def installFile(filepath, package, host, port):

    sz = installableELFSize(filepath)

    print("installing %s (%s, %d bytes)" % (filepath, package, sz))

    installer = Installer(host, port)
    response = installer.writeHeader(sz, package)

    installer.writeFileContents(sz, filepath)

    installer.commit()

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Interact with a CloudBase over the network.')
    parser.add_argument('command', help='install or delete')
    parser.add_argument('filepath', default='membrane.elf', help='path to the file to be installed or deleted')
    parser.add_argument('packageStr', default='com.sifteo.membrane', help='path to the file to be installed or deleted')
    #parser.add_argument('filepath', help='path to the file to be installed or deleted')
    #parser.add_argument('--ip', default='169.254.1.1', help='ip address to connect to')
    parser.add_argument('--ip', default='127.0.0.1', help='ip address to connect to')
    parser.add_argument('--port', default=2000, type=int, help='port to connect to')

    args = parser.parse_args()

    # we're taking a shortcut by requiring the user specify the package string,
    # rather than reading it ourselves
    installFile(args.filepath, args.packageStr, args.ip, args.port)

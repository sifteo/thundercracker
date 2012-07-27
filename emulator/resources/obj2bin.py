#!/usr/bin/env python
#
# Quick Wavefront OBJ to vertex buffer / index buffer converter.
#
# Assumes we have normals but not texture UVs. Generates one buffer:
#
#    - Number of vertices (uint16_t)
#    - Number of triangles (uint16_t)
#    - Padding to next 16-byte boundary
#    - For each vertex: float[3] normal, float[3] position
#    - Padding to next 16-byte boundary
#    - For each triangle: Three uint16_t indices
#
# M. Elizabeth Scott <beth@sifteo.com>
# 
# Copyright (c) 2012 Sifteo, Inc.
#

import struct
import sys


class OBJReader:
    def __init__(self):
        self.vertices = []
        self.normals = []
        self.vb = []
        self.vbMap = {}
        self.ib = []

    def read(self, f):
        for line in open(f, 'r'):
            tok = line.split('#', 1)[0].split()
            if not tok:
                continue

            if tok[0] == 'f':
                if len(tok) != 4:
                    raise ValueError("Face is not triangular")
                for v in tok[1:]:
                    self.convertVertex(v)

            elif tok[0] == 'v':
                x,y,z = map(float, tok[1:])
                self.vertices.append((-x, y, z))
                continue

            elif tok[0] == 'vn':
                x,y,z = map(float, tok[1:])
                self.normals.append((-x, y, z))
                continue

            elif tok[0] in ('s',):
                # Ignored
                pass
            else:
                raise ValueError("Unhandled line type in OBJ file %r" % tok[0])

    def convertVertex(self, v):
        tok = v.split('/')
        if len(tok) != 3:
            raise ValueError("Unhandled vertex format %r" % v)

        # Combine vertex normal and position
        values = self.normals[int(tok[2]) - 1] + self.vertices[int(tok[0]) - 1]

        # Convert to packed vertex buffer entry
        packed = struct.pack("ffffff", *values)

        # Add to vertex and index buffer, after deduplicating entire vertex

        if not packed in self.vbMap:
            self.vbMap[packed] = len(self.vb)
            self.vb.append(packed)

        self.ib.append(struct.pack("H", self.vbMap[packed]))


def pad(f):
    while f.tell() % 16:
        f.write(chr(0))


def convert(objFile, binFile):
    reader = OBJReader()
    reader.read(objFile)

    out = open(binFile, 'wb')
    out.write(struct.pack("HH", len(reader.vb), len(reader.ib) // 3))
    pad(out)
    out.write("".join(reader.vb))
    pad(out)
    out.write("".join(reader.ib))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print 'usage: obj2bin.py input.obj output.bin'
    else:
        convert(sys.argv[1], sys.argv[2])

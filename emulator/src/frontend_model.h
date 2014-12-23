/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Micah Elizabeth Scott <micah@misc.name>
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _FRONTEND_MODEL_H
#define _FRONTEND_MODEL_H

#include <stdint.h>


struct VertexT {
    float tx, ty;
    float vx, vy, vz;
};

struct VertexTN {
    float tx, ty;
    float nx, ny, nz;
    float vx, vy, vz;
};

struct VertexN {
    float nx, ny, nz;
    float vx, vy, vz;
};


/*
 * Simple abstraction for binary 3D models, converted with obj2bin.py
 */

class FrontendModel {
 public:
    struct Header {
        uint16_t numVertices;
        uint16_t numTriangles;
    };

    void init(const uint8_t *data) {
        this->data = data;
    }

    const Header& header() const {
        return *(const Header*) data;
    }

    const VertexN *vertexData() const {
        return (const VertexN *) (data + align(sizeof(Header)));
    }

    unsigned vertexDataSize() const {
        return header().numVertices * sizeof(VertexN);
    }

    const uint16_t *indexData() const {
        return (const uint16_t*) (data + align(sizeof(Header)) + align(vertexDataSize()));
    }

    unsigned indexDataSize() const {
        return header().numTriangles * 3 * sizeof(uint16_t);
    }

private:
    static unsigned align(unsigned x) {
        return (x + 15) & ~15;
    }

    const uint8_t *data;
};

#endif

/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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

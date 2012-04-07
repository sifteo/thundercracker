/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for asset image operations.
 */

#include <sifteo/abi.h>
#include "svmmemory.h"
#include "imagedecoder.h"
#include "vram.h"

extern "C" {


void _SYS_image_memDraw(uint16_t *dest, const _SYSAssetImage *im,
    unsigned dest_stride, unsigned frame)
{
    LOG(("Unimplemented! %s\n", __FUNCTION__));
}

void _SYS_image_memDrawRect(uint16_t *dest, const _SYSAssetImage *im,
    unsigned dest_stride, unsigned frame,
    struct _SYSInt2 *srcXY, struct _SYSInt2 *size)
{
    LOG(("Unimplemented! %s\n", __FUNCTION__));
}

void _SYS_image_BG0Draw(struct _SYSAttachedVideoBuffer *vbuf,
    const _SYSAssetImage *im, uint16_t addr, unsigned frame)
{
    ImageDecoder decoder;

    if (!SvmMemory::mapRAM(vbuf))
        return;
    if (!decoder.init(im, vbuf->cube))
        return;

    unsigned S = 18;                            // Destination stride
    unsigned blockX = decoder.getBlockSize();   // Natural block size for codec
    unsigned blockS = blockX * S;               // Dest stride per-block
    unsigned W = decoder.getWidth();            // Total width
    unsigned H = decoder.getHeight();           // Total height

    for (unsigned by = 0, byA = addr; by < H; by += blockX, byA += blockS)
        for (unsigned bx = 0, bxA = byA; bx < W; bx += blockX, bxA += blockX)
            for (unsigned y = by, yE = by + blockX, yA = bxA; y != H && y != yE; ++y, yA += S)
                for (unsigned x = bx, xE = bx + blockX, xA = yA; x != W && x != xE; ++x, ++xA) {
                    uint16_t tile = decoder.tile(x, y, frame);
                    VRAM::truncateWordAddr(xA);
                    VRAM::poke(vbuf->vbuf, xA, _SYS_TILE77(tile));
                }
}

void _SYS_image_BG0DrawRect(struct _SYSAttachedVideoBuffer *vbuf,
    const _SYSAssetImage *im, uint16_t addr, unsigned frame,
        struct _SYSInt2 *srcXY, struct _SYSInt2 *size)
{
    LOG(("Unimplemented! %s\n", __FUNCTION__));
}

void _SYS_image_BG1Draw(struct _SYSAttachedVideoBuffer *vbuf,
    const _SYSAssetImage *im, struct _SYSInt2 *destXY, unsigned frame)
{
    LOG(("Unimplemented! %s\n", __FUNCTION__));
}

void _SYS_image_BG1DrawRect(struct _SYSAttachedVideoBuffer *vbuf,
    const _SYSAssetImage *im, struct _SYSInt2 *destXY, unsigned frame,
        struct _SYSInt2 *srcXY, struct _SYSInt2 *size)
{
    LOG(("Unimplemented! %s\n", __FUNCTION__));
}

void _SYS_image_BG2Draw(struct _SYSAttachedVideoBuffer *vbuf,
    const _SYSAssetImage *im, uint16_t addr, unsigned frame)
{
    LOG(("Unimplemented! %s\n", __FUNCTION__));
}

void _SYS_image_BG2DrawRect(struct _SYSAttachedVideoBuffer *vbuf,
    const _SYSAssetImage *im, uint16_t addr, unsigned frame,
        struct _SYSInt2 *srcXY, struct _SYSInt2 *size)
{
    LOG(("Unimplemented! %s\n", __FUNCTION__));
}


}  // extern "C"

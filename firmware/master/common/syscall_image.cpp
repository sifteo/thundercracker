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
    ImageDecoder decoder;
    if (!decoder.init(im))
        return;

    ImageIter iter(decoder, frame);

    if (!SvmMemory::mapRAM(dest, iter.getDestBytes(dest_stride)))
        return;
    iter.copyToMem(dest, dest_stride);
}

void _SYS_image_memDrawRect(uint16_t *dest, const _SYSAssetImage *im,
    unsigned dest_stride, unsigned frame,
    struct _SYSInt2 *srcXY, struct _SYSInt2 *size)
{
    struct _SYSInt2 lSrcXY, lSize;
    if (!SvmMemory::copyROData(lSrcXY, srcXY))
        return;
    if (!SvmMemory::copyROData(lSize, size))
        return;

    ImageDecoder decoder;
    if (!decoder.init(im))
        return;

    ImageIter iter(decoder, frame, lSrcXY.x, lSrcXY.y, lSize.x, lSize.y);

    if (!SvmMemory::mapRAM(dest, iter.getDestBytes(dest_stride)))
        return;
    iter.copyToMem(dest, dest_stride);
}

void _SYS_image_BG0Draw(struct _SYSAttachedVideoBuffer *vbuf,
    const _SYSAssetImage *im, uint16_t addr, unsigned frame)
{
    if (!SvmMemory::mapRAM(vbuf))
        return;

    ImageDecoder decoder;
    if (!decoder.init(im, vbuf->cube))
        return;

    ImageIter iter(decoder, frame);
    iter.copyToVRAM(vbuf->vbuf, addr, _SYS_VRAM_BG0_WIDTH);
}

void _SYS_image_BG0DrawRect(struct _SYSAttachedVideoBuffer *vbuf,
    const _SYSAssetImage *im, uint16_t addr, unsigned frame,
        struct _SYSInt2 *srcXY, struct _SYSInt2 *size)
{
    if (!SvmMemory::mapRAM(vbuf))
        return;

    struct _SYSInt2 lSrcXY, lSize;
    if (!SvmMemory::copyROData(lSrcXY, srcXY))
        return;
    if (!SvmMemory::copyROData(lSize, size))
        return;

    ImageDecoder decoder;
    if (!decoder.init(im, vbuf->cube))
        return;

    ImageIter iter(decoder, frame, lSrcXY.x, lSrcXY.y, lSize.x, lSize.y);
    iter.copyToVRAM(vbuf->vbuf, addr, _SYS_VRAM_BG0_WIDTH);
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
    if (!SvmMemory::mapRAM(vbuf))
        return;

    ImageDecoder decoder;
    if (!decoder.init(im, vbuf->cube))
        return;

    ImageIter iter(decoder, frame);
    iter.copyToVRAM(vbuf->vbuf, addr, _SYS_VRAM_BG2_WIDTH);
}

void _SYS_image_BG2DrawRect(struct _SYSAttachedVideoBuffer *vbuf,
    const _SYSAssetImage *im, uint16_t addr, unsigned frame,
        struct _SYSInt2 *srcXY, struct _SYSInt2 *size)
{
    if (!SvmMemory::mapRAM(vbuf))
        return;

    struct _SYSInt2 lSrcXY, lSize;
    if (!SvmMemory::copyROData(lSrcXY, srcXY))
        return;
    if (!SvmMemory::copyROData(lSize, size))
        return;

    ImageDecoder decoder;
    if (!decoder.init(im, vbuf->cube))
        return;

    ImageIter iter(decoder, frame, lSrcXY.x, lSrcXY.y, lSize.x, lSize.y);
    iter.copyToVRAM(vbuf->vbuf, addr, _SYS_VRAM_BG2_WIDTH);
}


}  // extern "C"

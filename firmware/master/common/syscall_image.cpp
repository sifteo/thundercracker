/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for asset image operations.
 */

#include <sifteo/abi.h>
#include "svmmemory.h"
#include "svmruntime.h"
#include "imagedecoder.h"
#include "vram.h"

extern "C" {


void _SYS_image_memDraw(uint16_t *dest, _SYSCubeID destCID,
    const _SYSAssetImage *im, unsigned dest_stride, unsigned frame)
{
    ImageDecoder decoder;
    if (destCID == _SYS_CUBE_ID_INVALID) {
        // Relocation disabled
        if (!decoder.init(im))
            return SvmRuntime::fault(F_BAD_ASSET_IMAGE);
    } else {
        // Relocate to a specific cube (validated by decoder.init)
        if (!decoder.init(im, destCID))
            return SvmRuntime::fault(F_BAD_ASSET_IMAGE);
    }

    ImageIter iter(decoder, frame);

    if (!SvmMemory::mapRAM(dest, iter.getDestBytes(dest_stride)))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    iter.copyToMem(dest, dest_stride);
}

void _SYS_image_memDrawRect(uint16_t *dest, _SYSCubeID destCID,
    const _SYSAssetImage *im, unsigned dest_stride, unsigned frame,
    struct _SYSInt2 *srcXY, struct _SYSInt2 *size)
{
    struct _SYSInt2 lSrcXY, lSize;
    if (!SvmMemory::copyROData(lSrcXY, srcXY))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);
    if (!SvmMemory::copyROData(lSize, size))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    ImageDecoder decoder;
    if (destCID == _SYS_CUBE_ID_INVALID) {
        // Relocation disabled
        if (!decoder.init(im))
            return SvmRuntime::fault(F_BAD_ASSET_IMAGE);
    } else {
        // Relocate to a specific cube (validated by decoder.init)
        if (!decoder.init(im, destCID))
            return SvmRuntime::fault(F_BAD_ASSET_IMAGE);
    }

    ImageIter iter(decoder, frame, lSrcXY.x, lSrcXY.y, lSize.x, lSize.y);

    if (!SvmMemory::mapRAM(dest, iter.getDestBytes(dest_stride)))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    iter.copyToMem(dest, dest_stride);
}

void _SYS_image_BG0Draw(struct _SYSAttachedVideoBuffer *vbuf,
    const _SYSAssetImage *im, uint16_t addr, unsigned frame)
{
    if (!SvmMemory::mapRAM(vbuf))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    ImageDecoder decoder;
    if (!decoder.init(im, vbuf->cube))
        return SvmRuntime::fault(F_BAD_ASSET_IMAGE);

    ImageIter iter(decoder, frame);
    iter.copyToVRAM(vbuf->vbuf, addr, _SYS_VRAM_BG0_WIDTH);
}

void _SYS_image_BG0DrawRect(struct _SYSAttachedVideoBuffer *vbuf,
    const _SYSAssetImage *im, uint16_t addr, unsigned frame,
        struct _SYSInt2 *srcXY, struct _SYSInt2 *size)
{
    if (!SvmMemory::mapRAM(vbuf))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    struct _SYSInt2 lSrcXY, lSize;
    if (!SvmMemory::copyROData(lSrcXY, srcXY))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);
    if (!SvmMemory::copyROData(lSize, size))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    ImageDecoder decoder;
    if (!decoder.init(im, vbuf->cube))
        return SvmRuntime::fault(F_BAD_ASSET_IMAGE);

    ImageIter iter(decoder, frame, lSrcXY.x, lSrcXY.y, lSize.x, lSize.y);
    iter.copyToVRAM(vbuf->vbuf, addr, _SYS_VRAM_BG0_WIDTH);
}

void _SYS_image_BG1Draw(struct _SYSAttachedVideoBuffer *vbuf,
    const _SYSAssetImage *im, struct _SYSInt2 *destXY, unsigned frame)
{
    if (!SvmMemory::mapRAM(vbuf))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    struct _SYSInt2 lDestXY;
    if (!SvmMemory::copyROData(lDestXY, destXY))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    ImageDecoder decoder;
    if (!decoder.init(im, vbuf->cube))
        return SvmRuntime::fault(F_BAD_ASSET_IMAGE);

    ImageIter iter(decoder, frame);
    iter.copyToBG1(vbuf->vbuf, lDestXY.x, lDestXY.y);
}

void _SYS_image_BG1DrawRect(struct _SYSAttachedVideoBuffer *vbuf,
    const _SYSAssetImage *im, struct _SYSInt2 *destXY, unsigned frame,
        struct _SYSInt2 *srcXY, struct _SYSInt2 *size)
{
    if (!SvmMemory::mapRAM(vbuf))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    struct _SYSInt2 lDestXY, lSrcXY, lSize;
    if (!SvmMemory::copyROData(lDestXY, destXY))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);
    if (!SvmMemory::copyROData(lSrcXY, srcXY))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);
    if (!SvmMemory::copyROData(lSize, size))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    ImageDecoder decoder;
    if (!decoder.init(im, vbuf->cube))
        return SvmRuntime::fault(F_BAD_ASSET_IMAGE);

    ImageIter iter(decoder, frame, lSrcXY.x, lSrcXY.y, lSize.x, lSize.y);
    iter.copyToBG1(vbuf->vbuf, lDestXY.x, lDestXY.y);
}

void _SYS_image_BG1MaskedDraw(struct _SYSAttachedVideoBuffer *vbuf,
    const struct _SYSAssetImage *im, uint16_t key, unsigned frame)
{
    if (!SvmMemory::mapRAM(vbuf))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    ImageDecoder decoder;
    if (!decoder.init(im, vbuf->cube))
        return SvmRuntime::fault(F_BAD_ASSET_IMAGE);

    ImageIter iter(decoder, frame);
    iter.copyToBG1Masked(vbuf->vbuf, key);
}

void _SYS_image_BG1MaskedDrawRect(struct _SYSAttachedVideoBuffer *vbuf,
    const struct _SYSAssetImage *im, uint16_t key, unsigned frame,
    struct _SYSInt2 *srcXY, struct _SYSInt2 *size)
{
    if (!SvmMemory::mapRAM(vbuf))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    struct _SYSInt2 lSrcXY, lSize;
    if (!SvmMemory::copyROData(lSrcXY, srcXY))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);
    if (!SvmMemory::copyROData(lSize, size))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    ImageDecoder decoder;
    if (!decoder.init(im, vbuf->cube))
        return SvmRuntime::fault(F_BAD_ASSET_IMAGE);

    ImageIter iter(decoder, frame, lSrcXY.x, lSrcXY.y, lSize.x, lSize.y);
    iter.copyToBG1Masked(vbuf->vbuf, key);
}

void _SYS_image_BG2Draw(struct _SYSAttachedVideoBuffer *vbuf,
    const _SYSAssetImage *im, uint16_t addr, unsigned frame)
{
    if (!SvmMemory::mapRAM(vbuf))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    ImageDecoder decoder;
    if (!decoder.init(im, vbuf->cube))
        return SvmRuntime::fault(F_BAD_ASSET_IMAGE);

    ImageIter iter(decoder, frame);
    iter.copyToVRAM(vbuf->vbuf, addr, _SYS_VRAM_BG2_WIDTH);
}

void _SYS_image_BG2DrawRect(struct _SYSAttachedVideoBuffer *vbuf,
    const _SYSAssetImage *im, uint16_t addr, unsigned frame,
        struct _SYSInt2 *srcXY, struct _SYSInt2 *size)
{
    if (!SvmMemory::mapRAM(vbuf))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    struct _SYSInt2 lSrcXY, lSize;
    if (!SvmMemory::copyROData(lSrcXY, srcXY))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);
    if (!SvmMemory::copyROData(lSize, size))
        return SvmRuntime::fault(F_SYSCALL_ADDRESS);

    ImageDecoder decoder;
    if (!decoder.init(im, vbuf->cube))
        return SvmRuntime::fault(F_BAD_ASSET_IMAGE);

    ImageIter iter(decoder, frame, lSrcXY.x, lSrcXY.y, lSize.x, lSize.y);
    iter.copyToVRAM(vbuf->vbuf, addr, _SYS_VRAM_BG2_WIDTH);
}


}  // extern "C"

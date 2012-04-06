/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for asset image operations.
 */

#include <sifteo/abi.h>
#include "svmmemory.h"

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
    _SYSAssetImage header;
    if (!SvmMemory::copyROData(header, im))
        return;

    _SYS_vbuf_wrect(&vbuf->vbuf, addr,
        (const uint16_t*)(im+1) + (header.width * header.height * frame),
        0, header.width, header.height, header.width, 18);
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

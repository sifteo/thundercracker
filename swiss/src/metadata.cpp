/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * swiss - your Sifteo utility knife
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

#include "metadata.h"
#include "basedevice.h"
#include "usbvolumemanager.h"

Metadata::Metadata(IODevice &_dev) :
    dev(_dev)
{
}

std::string Metadata::getString(unsigned volBlockCode, unsigned key)
{
    USBProtocolMsg buf;
    BaseDevice base(dev);
    if (!base.getMetadata(buf, volBlockCode, key)) {
        return "(none)";
    }

    return std::string(buf.castPayload<char>());
}

int Metadata::getBytes(unsigned volBlockCode, unsigned key, uint8_t *buffer, unsigned len)
{
    USBProtocolMsg buf;
    BaseDevice base(dev);
    if (!base.getMetadata(buf, volBlockCode, key)) {
        return -1;
    }

    unsigned chunk = std::min(len, buf.len);
    memcpy(buffer, buf.castPayload<uint8_t>(), chunk);

    return chunk;
}

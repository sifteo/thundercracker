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

#include "inspect.h"
#include "installer.h"
#include "sifteo/abi/elf.h"

#include <errno.h>
#include <string>
#include <iomanip>
using namespace std;

int Inspect::run(int argc, char **argv, IODevice &_dev)
{
    if (argc < 2) {
        fprintf(stderr, "inspect - requires at least one argument\n");
        return false;
    }
    const char *path = argv[1];

    Inspect inspector;
    return inspector.inspect(path) ? 0 : 1;
}

bool Inspect::inspect(const char *path)
{
    /*
     * Look up all the metadata keys we know about,
     * and inspect their values within the given elf
     */

    if (!dbgInfo.init(path)) {
        fprintf(stderr, "couldn't open %s: %s\n", path, strerror(errno));
        return false;
    }

    TabularList table;

    // UUID
    uint32_t uuidLen;
    uint8_t *uuid = dbgInfo.metadata(_SYS_METADATA_UUID, uuidLen);
    table.cell() << "uuid:";
    table.cell() << (uuid ? uuidStr(*(_SYSUUID*)uuid) : "none");
    table.endRow();


    // Bootstrap Assets - included or not?
    uint32_t bootAssetLen;
    uint8_t *bootasset = dbgInfo.metadata(_SYS_METADATA_BOOT_ASSET, bootAssetLen);
    table.cell() << "boot asset:";
    table.cell() << (bootasset ? "included" : "none");
    table.endRow();


    // String values - title, package and version
    dumpString(table, _SYS_METADATA_TITLE_STR, "title");
    dumpString(table, _SYS_METADATA_PACKAGE_STR, "package");
    dumpString(table, _SYS_METADATA_VERSION_STR, "version");

    string fullApp;
    table.cell() << "is demo of:";
    if (dbgInfo.metadataString(_SYS_METADATA_IS_DEMO_OF_STR, fullApp)) {
        table.cell() << fullApp;
    } else {
        table.cell() << "none";
    }
    table.endRow();

    // Icon - is it included?
    uint32_t iconLen;
    uint8_t *icon = dbgInfo.metadata(_SYS_METADATA_ICON_96x96, iconLen);
    table.cell() << "icon:";
    table.cell() << (icon ? "included" : "none");
    table.endRow();


    // Number of assets slots used
    uint32_t aslotsLen;
    uint8_t *aslots = dbgInfo.metadata(_SYS_METADATA_NUM_ASLOTS, aslotsLen);
    unsigned numslots = (aslots && aslotsLen >= 1) ? *aslots : 0;
    table.cell() << "asset slots:";
    table.cell() << numslots;
    table.endRow();


    // Advertised cube range
    uint32_t cuberangeLen;
    uint8_t *cuberange = dbgInfo.metadata(_SYS_METADATA_CUBE_RANGE, cuberangeLen);

    table.cell() << "cube range:";
    if (cuberange) {
        _SYSMetadataCubeRange *cr = reinterpret_cast<_SYSMetadataCubeRange*>(cuberange);
        table.cell() << int(cr->minCubes) << "-" << int(cr->maxCubes);
    } else {
        table.cell() << "none";
    }
    table.endRow();


    // Minimum OS version required
    uint32_t minOSLen;
    uint8_t *mos = dbgInfo.metadata(_SYS_METADATA_MIN_OS_VERSION, minOSLen);
    table.cell() << "min OS version:";
    if (mos) {
        uint32_t minimumOS = *reinterpret_cast<uint32_t*>(mos);
        table.cell() << "0x" << setiosflags(ios::hex) << setw(6) << setfill('0') << minimumOS;
    } else {
        table.cell() << "none";
    }
    table.endRow();

    table.cell() << "installable size:";
    FILE *f = fopen(path, "rb");
    if (f) {
        unsigned sz = Installer::getInstallableElfSize(f);
        table.cell() << int(sz) << " bytes";
        fclose(f);
    } else {
        table.cell() << "unknown";
    }
    table.endRow();


    table.end();
    return true;
}

string Inspect::uuidStr(const _SYSUUID & u)
{
    char buf[0xff];
    // adapated from https://www.ietf.org/rfc/rfc4122
    int len = sprintf(buf, "%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
                      u.time_low,
                      u.time_mid,
                      u.time_hi_and_version,
                      u.clk_seq_hi_res,
                      u.clk_seq_low,
                      u.node[0],
                      u.node[1],
                      u.node[2],
                      u.node[3],
                      u.node[4],
                      u.node[5]);
    return string(buf, len);
}

void Inspect::dumpString(TabularList &tl, uint16_t key, const char *name)
{
    string s;
    dbgInfo.metadataString(key, s);
    tl.cell() << name << ":";
    tl.cell() << s;
    tl.endRow();
}

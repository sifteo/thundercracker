/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */
 
#include "cube_flash_storage.h"

namespace Cube {

bool FlashStorage::init(const char *filename)
{
    memset(&data, 0xFF, sizeof data);
    
    if (filename) {
        size_t result;
        
        file = fopen(filename, "rb+");
        if (!file)
            file = fopen(filename, "wb+");
        if (!file)
            return false;

        fprintf(stderr, "FLASH: Using file '%s'\n", filename);

        result = fread(&data, 1, sizeof data, file);
        if (result < 0)
            return false;

    }

    return true;
}

void FlashStorage::exit()
{
    if (write_timer)
        write();
    if (file)
        fclose(file);
}

void FlashStorage::write()
{
    if (file) {
        size_t result;
        
        /*
         * Rewrite the file without closing it.
         */

        fseek(file, 0, SEEK_SET);
        result = fwrite(&data, sizeof data, 1, file);
        if (result != 1) {
            perror("Error writing flash");
            return;
        }
        fflush(file);

        /*
         * Automatically trim out any unprogrammed bytes from the end of the buffer.
         */
        
        unsigned size = sizeof data;
        while (size && ((uint8_t*)&data)[size-1] == 0xFF)
            size--;
        ftruncate(fileno(file), size);

        fprintf(stderr, "FLASH: Flushed to disk\n");
    }
}

};  // namespace Cube

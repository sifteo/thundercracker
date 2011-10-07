/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _FLASH_H
#define _FLASH_H

#include <stdint.h>
#include "flash_model.h"

class Flash {
 public:

    enum busy_flag {
        BF_IDLE     = 0,
        BF_PROGRAM  = (1 << 0),
        BF_ERASE    = (1 << 1),
    };

    struct Pins {
        uint32_t  addr;       // IN
        uint8_t   oe;         // IN, active-low
        uint8_t   ce;         // IN, active-low
        uint8_t   we;         // IN, active-low
        uint8_t   data_in;    // IN
        
        uint8_t   data_drv;   // OUT, active-high
    };

    bool init(const char *filename=NULL) {
        if (filename) {
            size_t result;
            
            file = fopen(filename, "rb+");
            if (!file)
                file = fopen(filename, "wb+");
            if (!file)
                return false;

            result = fread(data, 1, FlashModel::SIZE, file);
            if (result < 0)
                return false;
 
        } else {
            file = NULL;
        }

        return true;
    }

    void exit() {
        if (write_timer)
            write();
        if (file)
            fclose(file);
    }

    uint32_t getCycleCount() {
        uint32_t c = cycle_count;
        cycle_count = 0;
        return c;
    }

    enum busy_flag getBusyFlag() {
        // These busy flags are only reset after they're read.
        enum busy_flag f = busy_status;
        busy_status = BF_IDLE;
        return f;
    }

    unsigned getBusyPercent() {
        uint32_t total_ticks = busy_ticks + idle_ticks;
        unsigned percent = total_ticks ? busy_ticks * 100 / total_ticks : 0;
        busy_ticks = 0;
        idle_ticks = 0;
        return percent;
    }

    const uint8_t *getData() {
        return data;
    }
    
    unsigned getSize() {
        return sizeof data;
    }
    
    void tick(struct em8051 *cpu) {
        /*
         * March time forward on the current operation, if any.
         */

        if (busy) {
            if (!--busy_timer) {
                busy = BF_IDLE;
                
                /*
                 * For performance tuning, it's useful to know where the
                 * flash first became idle.  If it was while we were
                 * waiting, great. If it was during flash decoding, we're
                 * wasting time!
                 */
                {
                    unsigned pc = cpu->mPC & (CODE_SIZE - 1);
                    struct profile_data *pd = &cpu->mProfilerMem[pc];
                    pd->flash_idle++;
                }
            }
            busy_ticks++;
        } else {
            idle_ticks++;
        }

        if (write_timer) {
            if (!--write_timer)
                write();
        }

        // Latch any busy flags long enough for the UI to see them.
        busy_status = (enum busy_flag) (busy_status | busy);
    }

    void cycle(Pins *pins) {
        if (pins->ce) {
            // Chip disabled
            pins->data_drv = 0;
            prev_we = 1;
            prev_oe = 1;
            
        } else {
            uint32_t addr = (FlashModel::SIZE - 1) & pins->addr;
            
            /*
             * Command writes are triggered on a falling WE edge.
             */

            if (!pins->we && prev_we) {
                cycle_count++;
                latched_addr = addr;

                cmd_fifo[cmd_fifo_head].addr = addr;
                cmd_fifo[cmd_fifo_head].data = pins->data_in;
                matchCommands();
                cmd_fifo_head = CMD_FIFO_MASK & (cmd_fifo_head + 1);
            }

            /*
             * Reads can occur on any cycle with OE asserted.
             *
             * If we're busy, the read will return a status
             * byte. Otherwise, it returns data from the memory array.
             *
             * For power consumption accounting purposes, this counts as a
             * read cycle if OE/CE were just now asserted, OR if the
             * address changed.
             */

            if (pins->oe) {
                pins->data_drv = 0;
            } else {

                // Toggle bits only change on an OE edge.
                if (prev_oe)
                    updateStatusByte();

                pins->data_drv = 1;
                if (addr != latched_addr || prev_oe) {
                    cycle_count++;
                    latched_addr = addr;
                }
            }
            
            prev_we = pins->we;
            prev_oe = pins->oe;
        }
    }

    uint8_t dataOut() {
        /*
         * On every flash_cycle() we may compute a new value for
         * data_drv. But the data port itself may change values in-between
         * cycles, e.g. on an erase/program completion. This function is
         * invoked more frequently (every tick) by hardware.c, in order to
         * update the flash data when data_drv is asserted.
         */
        return busy ? status_byte :
            data[latched_addr];
    }

 private:
    void write() {
        if (file) {
            size_t result;
            
            fseek(file, 0, SEEK_SET);
            
            result = fwrite(data, FlashModel::SIZE, 1, file);
            if (result != 1)
                perror("Error writing flash");
            
            fflush(file);
        }
    }
    
    bool matchCommand(const FlashModel::command_sequence *seq) {
        unsigned i;
        uint8_t fifo_index = cmd_fifo_head - FlashModel::CMD_LENGTH + 1;
   
        for (i = 0; i < FlashModel::CMD_LENGTH; i++, fifo_index++) {
            fifo_index &= CMD_FIFO_MASK;

            if ( (cmd_fifo[fifo_index].addr & seq[i].addr_mask) != seq[i].addr ||
                 (cmd_fifo[fifo_index].data & seq[i].data_mask) != seq[i].data )
                return false;
        }

        return true;
    }

    void erase(unsigned addr, unsigned size) {
        memset(data + (addr & ~(size - 1)), 0xFF, size);
    }

    void matchCommands() {
        struct cmd_state *st = &cmd_fifo[cmd_fifo_head];

        if (busy != BF_IDLE)
            return;

        if (matchCommand(FlashModel::cmd_byte_program)) {
            data[st->addr] &= st->data;
            status_byte = FlashModel::STATUS_DATA_INV & ~st->data;
            busy = BF_PROGRAM;
            write_timer = USEC_TO_CYCLES(FLUSH_TIME_US);
            busy_timer = USEC_TO_CYCLES(FlashModel::PROGRAM_TIME_US);
        
        } else if (matchCommand(FlashModel::cmd_sector_erase)) {
            erase(st->addr, FlashModel::SECTOR_SIZE);
            status_byte = 0;
            busy = BF_ERASE;
            write_timer = USEC_TO_CYCLES(FLUSH_TIME_US);
            busy_timer = USEC_TO_CYCLES(FlashModel::ERASE_SECTOR_TIME_US);
            
        } else if (matchCommand(FlashModel::cmd_block_erase)) {
            erase(st->addr, FlashModel::BLOCK_SIZE);
            status_byte = 0;
            busy = BF_ERASE;
            write_timer = USEC_TO_CYCLES(FLUSH_TIME_US);
            busy_timer = USEC_TO_CYCLES(FlashModel::ERASE_BLOCK_TIME_US);
            
        } else if (matchCommand(FlashModel::cmd_chip_erase)) {
            erase(st->addr, FlashModel::SIZE);
            status_byte = 0;
            busy = BF_ERASE;
            write_timer = USEC_TO_CYCLES(FLUSH_TIME_US);
            busy_timer = USEC_TO_CYCLES(FlashModel::ERASE_CHIP_TIME_US);
        }
    }

    void updateStatusByte() {
        status_byte ^= FlashModel::STATUS_TOGGLE;

        if (busy == BF_ERASE)
            status_byte ^= FlashModel::STATUS_ERASE_TOGGLE;
    }

    static const uint8_t CMD_FIFO_MASK = 0xF;
 
    /*
     * How long (in microseconds of virtual time) to wait for
     * writes/erases to cease before we rewrite the flash file on disk.
     *
     * This is a simple way for us to write the flash to disk in a
     * non-error-prone way while also avoiding a lot of unnecessary
     * write() calls, or platform-specific memory mapping.
     */
    static const unsigned FLUSH_TIME_US = 100000;

    struct cmd_state {
        uint32_t addr;
        uint8_t data;
    };

    // Disk I/O
    uint32_t write_timer;
    FILE *file;

    // For clock speed / power metrics
    uint32_t cycle_count;
    uint32_t write_count;
    uint32_t erase_count;
    uint32_t busy_ticks;
    uint32_t idle_ticks;
    enum busy_flag busy_status;

    // Command state
    uint32_t latched_addr;
    uint32_t busy_timer;
    enum busy_flag busy;
    uint8_t cmd_fifo_head;
    uint8_t prev_we;
    uint8_t prev_oe;
    uint8_t status_byte;
    struct cmd_state cmd_fifo[CMD_FIFO_MASK + 1];

    // Memory array
    uint8_t data[FlashModel::SIZE];
};

#endif

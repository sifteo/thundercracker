/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBE_FLASH_H
#define _CUBE_FLASH_H

#include <stdint.h>

#include "vtime.h"
#include "cube_cpu.h"
#include "cube_flash_model.h"
#include "flash_storage.h"
#include "tracer.h"

namespace Cube {


class Flash {
 public:
    enum busy_flag {
        BF_IDLE             = 0,
        BF_PROGRAM_BYTE     = (1 << 0),
        BF_PROGRAM_BUFFER   = (1 << 1),
        BF_ERASE_SECTOR     = (1 << 2),
        BF_ERASE_CHIP       = (1 << 3),

        BF_PROGRAM          = (BF_PROGRAM_BYTE | BF_PROGRAM_BUFFER),
        BF_ERASE            = (BF_ERASE_SECTOR | BF_ERASE_CHIP),
    };

    struct Pins {
        uint32_t  addr;       // IN
        uint8_t   power;      // IN, active-high
        uint8_t   oe;         // IN, active-low
        uint8_t   ce;         // IN, active-low
        uint8_t   we;         // IN, active-low
        uint8_t   data_in;    // IN
        
        uint8_t   data_drv;   // OUT, active-high
    };

    void init(FlashStorage::CubeRecord *_storage) {
        storage = _storage;
        
        cycle_count = 0;
        write_count = 0;
        erase_count = 0;
        busy_ticks = 0;
        idle_ticks = 0;
        busy_status = BF_IDLE;
        latched_addr = 0;
        busy_timer = 0;
        busy = BF_IDLE;
        cmd_fifo_head = 0;
        prev_we = 0;
        prev_oe = 0;
        status_byte = 0;
        previous_clocks = 0;
        buffer_counter = 0;
    }

    FlashStorage::CubeRecord *getStorage() const {
        return storage;
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

    uint32_t getWriteCount() {
        return write_count;
    }

    uint32_t getEraseCount() {
        return erase_count;
    }

    uint32_t getModifyCount() {
        return write_count + erase_count;
    }

    ALWAYS_INLINE void tick(TickDeadline &deadline, CPU::em8051 *cpu) {
        /*
         * March time forward on the current operation, if any.
         */

        uint64_t elapsed = deadline.clock() - previous_clocks;
        previous_clocks = deadline.clock();

        if (UNLIKELY(busy)) {
            // Latch any busy flags long enough for the UI to see them.
            busy_status = (enum busy_flag) (busy_status | busy);

            if (busy_timer == 0) {
                /*
                 * We just became busy, and haven't set the timer yet.
                 */

                cpu->needHardwareTick = true;
                
                switch (busy) {
                case BF_PROGRAM_BYTE:
                    busy_timer = deadline.setRelative(VirtualTime::usec(FlashModel::PROGRAM_BYTE_TIME_US));
                    break;
                case BF_PROGRAM_BUFFER:
                    busy_timer = deadline.setRelative(VirtualTime::usec(FlashModel::PROGRAM_BUFFER_TIME_US));
                    break;
                case BF_ERASE_SECTOR:
                    busy_timer = deadline.setRelative(VirtualTime::usec(FlashModel::ERASE_SECTOR_TIME_US));
                    break;
                case BF_ERASE_CHIP:
                    busy_timer = deadline.setRelative(VirtualTime::usec(FlashModel::ERASE_CHIP_TIME_US));
                    break;
                default:
                    break;
                }

            } else if (deadline.hasPassed(busy_timer)) {
                /*
                 * Just finished our operation
                 */

                Tracer::log(cpu, "FLASH: no longer busy");

                busy = BF_IDLE;
                busy_timer = 0;
                
                /*
                 * For performance tuning, it's useful to know where the
                 * flash first became idle.  If it was while we were
                 * waiting, great. If it was during flash decoding, we're
                 * wasting time!
                 */
                if (cpu->mProfileData) {
                    unsigned pc = cpu->mPC & PC_MASK;
                    CPU::profile_data *pd = &cpu->mProfileData[pc];
                    pd->flash_idle++;
                }

            } else {
                /*
                 * Still busy...
                 */

                deadline.set(busy_timer);
            }

            busy_ticks += elapsed;

        } else {
            idle_ticks += elapsed;
        }
    }

    ALWAYS_INLINE void cycle(Pins *pins, CPU::em8051 *cpu) {
        if (pins->ce || !pins->power) {
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
                matchCommands(cpu);
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

                // It's a mistake to try reading during a buffer write command
                if (buffer_counter)
                    CPU::except(cpu, CPU::EXCEPTION_FLASH_BUSY);

                // Toggle bits only change on an OE edge.
                if (prev_oe)
                    updateStatusByte();

                pins->data_drv = 1;
                if (addr != latched_addr || prev_oe) {
                    cycle_count++;
                    latched_addr = addr;

                    Tracer::log(cpu, "FLASH: read addr [%06x] -> %02x (busy=%d)",
                        addr, dataOut(), busy);
                }
            }
            
            prev_we = pins->we;
            prev_oe = pins->oe;
        }
    }

    ALWAYS_INLINE uint8_t dataOut() {
        /*
         * On every flash_cycle() we may compute a new value for
         * data_drv. But the data port itself may change values in-between
         * cycles, e.g. on an erase/program completion. This function is
         * invoked more frequently (every tick) by hardware.c, in order to
         * update the flash data when data_drv is asserted.
         */

        if (UNLIKELY(busy | buffer_counter))
            return status_byte;

        return storage->ext[latched_addr];
    }

 private:
    
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
        addr &= ~(size - 1);
        ASSERT(addr + size <= sizeof storage->ext);
        memset(storage->ext + addr, 0xFF, size);

        unsigned sBegin = addr / FlashModel::SECTOR_SIZE;
        unsigned sEnd = (addr + size) / FlashModel::SECTOR_SIZE;
        for (unsigned s = sBegin; s != sEnd; ++s)
            storage->eraseCounts[s]++;
    }

    void handleBufferWrite(CPU::em8051 *cpu) {
        /*
         * Validate the contents of the command buffer, and start the write if it's okay.
         *
         * We've already checked the first four cycles (command & sector addresses),
         * as well as the sector address for the confirm. We still need to check:
         *
         *   - The confirm byte itself
         *   - All buffer addresses
         *
         * First, dump out the entire buffer for tracing.
         */

        Tracer::log(cpu, "FLASH: Last cycle of %d-byte buffer write. Buffer contents:", buffer_bytes);

        for (unsigned i = 0; i <= buffer_bytes; i++) {
            struct cmd_state *st = &cmd_fifo[(cmd_fifo_head - buffer_bytes + i) & CMD_FIFO_MASK];
            Tracer::log(cpu, "FLASH: Buffer[%d] addr=%06x data=%02x", i, st->addr, st->data);
        }

        // Check confirmation byte
        if (cmd_fifo[cmd_fifo_head].data != 0x29) {
            Tracer::log(cpu, "FLASH: bad confirmation byte");
            CPU::except(cpu, CPU::EXCEPTION_FLASH_CMD);
            return;
        }

        /*
         * Strict validation for addresses.
         * (The real chip doesn't actually care that addresses are
         * sequential, but we always will want them to be.)
         */
        for (unsigned i = 0; i <= buffer_bytes; i++) {
            struct cmd_state *st = &cmd_fifo[(cmd_fifo_head - buffer_bytes + i) & CMD_FIFO_MASK];
            
            if (i < buffer_bytes && bufOffsetAddr(st->addr) != i) {
                Tracer::log(cpu, "FLASH: addresses in [%d] not sequential as expected", i);
                CPU::except(cpu, CPU::EXCEPTION_FLASH_CMD);
                return;
            }

            if (sectorAddr(st->addr) != buffer_sa) {
                Tracer::log(cpu, "FLASH: sector mismatch in [%d], sa=%06x", i, buffer_sa);
                CPU::except(cpu, CPU::EXCEPTION_FLASH_CMD);
                return;
            }

            if (bufPageAddr(st->addr) != bufPageAddr(cmd_fifo[cmd_fifo_head].addr)) {
                Tracer::log(cpu, "FLASH: page mismatch in [%d] (%06x != %06x)",
                    i, bufPageAddr(st->addr), bufPageAddr((cmd_fifo[cmd_fifo_head].addr)));
                CPU::except(cpu, CPU::EXCEPTION_FLASH_CMD);
                return;
            }
        }

        /*
         * Now do the program, all at once.
         */
        for (unsigned i = 0; i < buffer_bytes; i++) {
            struct cmd_state *st = &cmd_fifo[(cmd_fifo_head - buffer_bytes + i) & CMD_FIFO_MASK];
            storage->ext[st->addr] &= st->data;
            status_byte = FlashModel::STATUS_DATA_INV & ~st->data;
        }

        write_count += buffer_bytes;
        busy = BF_PROGRAM_BUFFER;
    }

    void matchCommands(CPU::em8051 *cpu) {
        struct cmd_state *st = &cmd_fifo[cmd_fifo_head];

        // Busy? (In a self-timed program/erase operation)
        if (busy != BF_IDLE) {
            ASSERT(buffer_counter == 0);
            CPU::except(cpu, CPU::EXCEPTION_FLASH_BUSY);
            return;
        }

        // Counting down to the end of a buffer-write command?
        if (buffer_counter) {
            Tracer::log(cpu, "FLASH: buffer countdown, %d", buffer_counter);
            if (buffer_sa == sectorAddr(st->addr)) {
                if (!--buffer_counter) {
                    // Finished buffer.
                    handleBufferWrite(cpu);
                }
                return;
            } else {
                Tracer::log(cpu, "FLASH: sector addr mismatch during buffer write sa=%06x addr=%06x",
                    buffer_sa, st->addr);
                CPU::except(cpu, CPU::EXCEPTION_FLASH_CMD);
                buffer_counter = 0;
            }
        }

        if (matchCommand(FlashModel::cmd_buffer_begin)) {
            struct cmd_state *stPrev = &cmd_fifo[(cmd_fifo_head - 1) & CMD_FIFO_MASK];

            Tracer::log(cpu, "FLASH: begin buffer program, addr [%06x, %06x] count %d",
                stPrev->addr, st->addr, st->data);

            buffer_bytes = st->data + 1;            // Parameter is byte count - 1
            buffer_counter = buffer_bytes + 1;      // One extra clock cycle for confirm cmd
            buffer_sa = sectorAddr(st->addr);

            // Make sure the buffer size is in range, and the sector addresses match.
            if (buffer_bytes > FlashModel::BUFFER_SIZE
                || buffer_sa != sectorAddr(stPrev->addr)) {
                // Buffer is too large
                CPU::except(cpu, CPU::EXCEPTION_FLASH_CMD);
                buffer_counter = 0;
            }

        } else if (matchCommand(FlashModel::cmd_byte_program)) {
            Tracer::log(cpu, "FLASH: programming addr [%06x], %02x -> %02x",
                st->addr, storage->ext[st->addr], st->data);

            storage->ext[st->addr] &= st->data;
            status_byte = FlashModel::STATUS_DATA_INV & ~st->data;
            busy = BF_PROGRAM_BYTE;
            write_count++;

        } else if (matchCommand(FlashModel::cmd_sector_erase)) {
            Tracer::log(cpu, "FLASH: sector erase [%06x]", st->addr);

            erase(st->addr, FlashModel::SECTOR_SIZE);
            status_byte = 0;
            busy = BF_ERASE_SECTOR;
            erase_count++;

        } else if (matchCommand(FlashModel::cmd_chip_erase)) {
            Tracer::log(cpu, "FLASH: chip erase [%06x]", st->addr);

            erase(st->addr, FlashModel::SIZE);
            status_byte = 0;
            busy = BF_ERASE_CHIP;
            erase_count++;
        }
    }

    void updateStatusByte() {
        status_byte ^= FlashModel::STATUS_TOGGLE;

        if (busy == BF_ERASE)
            status_byte ^= FlashModel::STATUS_ERASE_TOGGLE;
    }
    
    static ALWAYS_INLINE uint32_t sectorAddr(uint32_t fullAddr) {
        return fullAddr & ~(FlashModel::SECTOR_SIZE - 1);
    }

    static ALWAYS_INLINE uint32_t bufPageAddr(uint32_t fullAddr) {
        return fullAddr & ~(FlashModel::BUFFER_SIZE - 1);
    }

    static ALWAYS_INLINE uint32_t bufOffsetAddr(uint32_t fullAddr) {
        return fullAddr & (FlashModel::BUFFER_SIZE - 1);
    }

    // Power of two, and large enough to hold the longest possible buffer-program op
    static const uint8_t CMD_FIFO_MASK = 0x3F;
 
    struct cmd_state {
        uint32_t addr;
        uint8_t data;
    };

    FlashStorage::CubeRecord *storage;

    // For clock speed / power metrics
    uint32_t cycle_count;
    uint32_t write_count;
    uint32_t erase_count;
    uint32_t busy_ticks;
    uint32_t idle_ticks;
    enum busy_flag busy_status;
    uint64_t previous_clocks;

    // Command state
    uint32_t latched_addr;
    uint64_t busy_timer;
    enum busy_flag busy;
    uint8_t cmd_fifo_head;
    uint8_t buffer_counter;
    uint8_t buffer_bytes;
    uint8_t prev_we;
    uint8_t prev_oe;
    uint8_t status_byte;
    uint32_t buffer_sa;
    struct cmd_state cmd_fifo[CMD_FIFO_MASK + 1];
};


};  // namespace Cube

#endif

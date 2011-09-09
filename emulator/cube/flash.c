/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "emulator.h"
#include "flash.h"
#include "flash_model.h"

#define CMD_FIFO_MASK  0xF

/*
 * How long (in microseconds of virtual time) to wait for
 * writes/erases to cease before we rewrite the flash file on disk.
 *
 * This is a simple way for us to write the flash to disk in a
 * non-error-prone way while also avoiding a lot of unnecessary
 * write() calls, or platform-specific memory mapping.
 */
#define FLUSH_TIME_US  100000

struct cmd_state {
    uint32_t addr;
    uint8_t data;
};

struct {
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
    uint8_t data[FLASH_SIZE];
} fl;


void flash_init(const char *filename)
{
    if (filename) {
	size_t result;

	fl.file = fopen(filename, "rb+");
	if (!fl.file) {
	    fl.file = fopen(filename, "wb+");
	}
	if (!fl.file) {
	    perror("Error opening flash");
	    exit(1);
	}

	result = fread(fl.data, 1, FLASH_SIZE, fl.file);
	if (result < 0) {
	    perror("Error reading flash");
	    exit(1);
	}	
 
    } else {
	fl.file = NULL;
    }
}

static void flash_write(void)
{
    if (fl.file) {
	size_t result;
	
	fseek(fl.file, 0, SEEK_SET);

	result = fwrite(fl.data, FLASH_SIZE, 1, fl.file);
	if (result != 1)
	    perror("Error writing flash");

	fflush(fl.file);
    }
}

void flash_exit(void)
{
    if (fl.write_timer)
	flash_write();
    if (fl.file)
	fclose(fl.file);
}

uint32_t flash_cycle_count(void)
{
    uint32_t c = fl.cycle_count;
    fl.cycle_count = 0;
    return c;
}

enum busy_flag flash_busy_flag(void)
{
    // These busy flags are only reset after they're read.
    enum busy_flag f = fl.busy_status;
    fl.busy_status = 0;
    return f;
}

unsigned flash_busy_percent(void)
{
    uint32_t total_ticks = fl.busy_ticks + fl.idle_ticks;
    unsigned percent = total_ticks ? fl.busy_ticks * 100 / total_ticks : 0;
    fl.busy_ticks = 0;
    fl.idle_ticks = 0;
    return percent;
}

static int flash_match_command(struct command_sequence *seq)
{
    unsigned i;
    uint8_t fifo_index = fl.cmd_fifo_head - CMD_LENGTH + 1;
   
    for (i = 0; i < CMD_LENGTH; i++, fifo_index++) {
	fifo_index &= CMD_FIFO_MASK;

	if ( (fl.cmd_fifo[fifo_index].addr & seq[i].addr_mask) != seq[i].addr ||
	     (fl.cmd_fifo[fifo_index].data & seq[i].data_mask) != seq[i].data )
	    return 0;
    }

    return 1;
}

static void flash_match_commands(void)
{
    struct cmd_state *st = &fl.cmd_fifo[fl.cmd_fifo_head];

    if (fl.busy != BF_IDLE)
	return;

    if (flash_match_command(cmd_byte_program)) {
	fl.data[st->addr] &= st->data;
	fl.status_byte = STATUS_DATA_INV & ~st->data;
	fl.busy = BF_PROGRAM;
	fl.write_timer = USEC_TO_CYCLES(FLUSH_TIME_US);
	fl.busy_timer = USEC_TO_CYCLES(PROGRAM_TIME_US);
	
    } else if (flash_match_command(cmd_sector_erase)) {
	memset(fl.data + (st->addr & ~(SECTOR_SIZE - 1)), 0xFF, SECTOR_SIZE);
	fl.status_byte = 0;
	fl.busy = BF_ERASE;
	fl.write_timer = USEC_TO_CYCLES(FLUSH_TIME_US);
	fl.busy_timer = USEC_TO_CYCLES(ERASE_SECTOR_TIME_US);
	
    } else if (flash_match_command(cmd_block_erase)) {
	memset(fl.data + (st->addr & ~(BLOCK_SIZE - 1)), 0xFF, BLOCK_SIZE);
	fl.status_byte = 0;
	fl.busy = BF_ERASE;
	fl.write_timer = USEC_TO_CYCLES(FLUSH_TIME_US);
	fl.busy_timer = USEC_TO_CYCLES(ERASE_BLOCK_TIME_US);

    } else if (flash_match_command(cmd_chip_erase)) {
	memset(fl.data, 0xFF, FLASH_SIZE);
	fl.status_byte = 0;
	fl.busy = BF_ERASE;
	fl.write_timer = USEC_TO_CYCLES(FLUSH_TIME_US);
	fl.busy_timer = USEC_TO_CYCLES(ERASE_CHIP_TIME_US);
    }
}

static void flash_update_status_byte(void)
{
    fl.status_byte ^= STATUS_TOGGLE;

    if (fl.busy == BF_ERASE)
	fl.status_byte ^= STATUS_ERASE_TOGGLE;
}

void flash_tick(struct em8051 *cpu)
{
    /*
     * March time forward on the current operation, if any.
     */

    if (fl.busy) {
	if (!--fl.busy_timer) {
	    fl.busy = BF_IDLE;

	    /*
	     * For performance tuning, it's useful to know where the
	     * flash first became idle.  If it was while we were
	     * waiting, great. If it was during flash decoding, we're
	     * wasting time!
	     */
	    {
		unsigned pc = cpu->mPC & (cpu->mCodeMemSize - 1);
		struct profile_data *pd = &cpu->mProfilerMem[pc];
		pd->flash_idle++;
	    }
	}
	fl.busy_ticks++;
    } else {
	fl.idle_ticks++;
    }

    if (fl.write_timer) {
	if (!--fl.write_timer)
	    flash_write();
    }

    // Latch any busy flags long enough for the UI to see them.
    fl.busy_status |= fl.busy;
}

void flash_cycle(struct flash_pins *pins)
{
    if (pins->ce) {
	// Chip disabled
	pins->data_drv = 0;
	fl.prev_we = 1;
	fl.prev_oe = 1;

    } else {
	uint32_t addr = (FLASH_SIZE - 1) & pins->addr;

	/*
	 * Command writes are triggered on a falling WE edge.
	 */

	if (!pins->we && fl.prev_we) {
	    fl.cycle_count++;
	    fl.latched_addr = addr;

	    fl.cmd_fifo[fl.cmd_fifo_head].addr = addr;
	    fl.cmd_fifo[fl.cmd_fifo_head].data = pins->data_in;
	    flash_match_commands();
	    fl.cmd_fifo_head = CMD_FIFO_MASK & (fl.cmd_fifo_head + 1);
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
	    if (fl.prev_oe)
		flash_update_status_byte();

	    pins->data_drv = 1;
	    if (addr != fl.latched_addr || fl.prev_oe) {
		fl.cycle_count++;
		fl.latched_addr = addr;
	    }
	}

	fl.prev_we = pins->we;
	fl.prev_oe = pins->oe;
    }
}

uint8_t flash_data_out(void)
{
    /*
     * On every flash_cycle() we may compute a new value for
     * data_drv. But the data port itself may change values in-between
     * cycles, e.g. on an erase/program completion. This function is
     * invoked more frequently (every tick) by hardware.c, in order to
     * update the flash data when data_drv is asserted.
     */
    return fl.busy ? fl.status_byte :
	fl.data[fl.latched_addr];
}

uint32_t flash_size(void)
{
    return sizeof fl.data;
}

uint8_t *flash_buffer(void)
{
    return fl.data;
}

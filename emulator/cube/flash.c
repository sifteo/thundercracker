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
    uint32_t cycle_prev_addr;
    uint32_t cycle_count;
    uint32_t write_count;
    uint32_t erase_count;
    uint32_t busy_ticks;
    uint32_t idle_ticks;
    enum busy_flag busy_status;

    // Command state
    uint32_t busy_timer;
    enum busy_flag busy;
    uint8_t cmd_fifo_head;
    uint8_t prev_we;
    uint8_t prev_oe;
    uint8_t status_byte;
    struct cmd_state cmd_fifo[CMD_FIFO_MASK + 1];

    // Memory array
    uint8_t data[FLASH_SIZE];
} flashmem;


void flash_init(const char *filename)
{
    if (filename) {
	size_t result;

	flashmem.file = fopen(filename, "rb+");
	if (!flashmem.file) {
	    flashmem.file = fopen(filename, "wb+");
	}
	if (!flashmem.file) {
	    perror("Error opening flash");
	    exit(1);
	}

	result = fread(flashmem.data, 1, FLASH_SIZE, flashmem.file);
	if (result < 0) {
	    perror("Error reading flash");
	    exit(1);
	}	
 
    } else {
	flashmem.file = NULL;
    }
}

static void flash_write(void)
{
    if (flashmem.file) {
	size_t result;
	
	fseek(flashmem.file, 0, SEEK_SET);

	result = fwrite(flashmem.data, FLASH_SIZE, 1, flashmem.file);
	if (result != 1)
	    perror("Error writing flash");

	fflush(flashmem.file);
    }
}

void flash_exit(void)
{
    if (flashmem.write_timer)
	flash_write();
    if (flashmem.file)
	fclose(flashmem.file);
}

uint32_t flash_cycle_count(void)
{
    uint32_t c = flashmem.cycle_count;
    flashmem.cycle_count = 0;
    return c;
}

enum busy_flag flash_busy_flag(void)
{
    // These busy flags are only reset after they're read.
    enum busy_flag f = flashmem.busy_status;
    flashmem.busy_status = 0;
    return f;
}

unsigned flash_busy_percent(void)
{
    uint32_t total_ticks = flashmem.busy_ticks + flashmem.idle_ticks;
    unsigned percent = total_ticks ? flashmem.busy_ticks * 100 / total_ticks : 0;
    flashmem.busy_ticks = 0;
    flashmem.idle_ticks = 0;
    return percent;
}

static int flash_match_command(struct command_sequence *seq)
{
    unsigned i;
    uint8_t fifo_index = flashmem.cmd_fifo_head - CMD_LENGTH + 1;
   
    for (i = 0; i < CMD_LENGTH; i++, fifo_index++) {
	fifo_index &= CMD_FIFO_MASK;

	if ( (flashmem.cmd_fifo[fifo_index].addr & seq[i].addr_mask) != seq[i].addr ||
	     (flashmem.cmd_fifo[fifo_index].data & seq[i].data_mask) != seq[i].data )
	    return 0;
    }

    return 1;
}

static void flash_match_commands(void)
{
    struct cmd_state *st = &flashmem.cmd_fifo[flashmem.cmd_fifo_head];

    if (flashmem.busy != BF_IDLE)
	return;

    if (flash_match_command(cmd_byte_program)) {
	flashmem.data[st->addr] &= st->data;
	flashmem.status_byte = STATUS_DATA_INV & ~st->data;
	flashmem.busy = BF_PROGRAM;
	flashmem.write_timer = USEC_TO_CYCLES(FLUSH_TIME_US);
	flashmem.busy_timer = USEC_TO_CYCLES(PROGRAM_TIME_US);
	
    } else if (flash_match_command(cmd_sector_erase)) {
	memset(flashmem.data + (st->addr & ~(SECTOR_SIZE - 1)), 0xFF, SECTOR_SIZE);
	flashmem.busy = BF_ERASE;
	flashmem.write_timer = USEC_TO_CYCLES(FLUSH_TIME_US);
	flashmem.busy_timer = USEC_TO_CYCLES(ERASE_SECTOR_TIME_US);
	
    } else if (flash_match_command(cmd_block_erase)) {
	memset(flashmem.data + (st->addr & ~(BLOCK_SIZE - 1)), 0xFF, BLOCK_SIZE);
	flashmem.busy = BF_ERASE;
	flashmem.write_timer = USEC_TO_CYCLES(FLUSH_TIME_US);
	flashmem.busy_timer = USEC_TO_CYCLES(ERASE_BLOCK_TIME_US);

    } else if (flash_match_command(cmd_chip_erase)) {
	memset(flashmem.data, 0xFF, FLASH_SIZE);
	flashmem.busy = BF_ERASE;
	flashmem.write_timer = USEC_TO_CYCLES(FLUSH_TIME_US);
	flashmem.busy_timer = USEC_TO_CYCLES(ERASE_CHIP_TIME_US);
    }
}

static void flash_update_status_byte(void)
{
    flashmem.status_byte ^= STATUS_TOGGLE;

    if (flashmem.busy == BF_ERASE)
	flashmem.status_byte ^= STATUS_ERASE_TOGGLE;
}

void flash_tick(struct em8051 *cpu)
{
    /*
     * March time forward on the current operation, if any.
     */

    if (flashmem.busy) {
	if (!--flashmem.busy_timer) {
	    flashmem.busy = BF_IDLE;

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
	flashmem.busy_ticks++;
    } else {
	flashmem.idle_ticks++;
    }

    if (flashmem.write_timer) {
	if (!--flashmem.write_timer)
	    flash_write();
    }

    // Latch any busy flags long enough for the UI to see them.
    flashmem.busy_status |= flashmem.busy;
}

void flash_cycle(struct flash_pins *pins)
{
    if (pins->ce) {
	// Chip disabled
	pins->data_drv = 0;
	flashmem.cycle_prev_addr = -1;
	flashmem.prev_we = 1;

    } else {
	uint32_t addr = (FLASH_SIZE - 1) & pins->addr;

	/*
	 * Command writes are triggered on a falling WE edge.
	 */

	if (!pins->we && flashmem.prev_we) {
	    flashmem.cmd_fifo[flashmem.cmd_fifo_head].addr = addr;
	    flashmem.cmd_fifo[flashmem.cmd_fifo_head].data = pins->data_in;
	    flash_match_commands();
	    flashmem.cmd_fifo_head = CMD_FIFO_MASK & (flashmem.cmd_fifo_head + 1);
	    flashmem.cycle_count++;
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
	    flashmem.cycle_prev_addr = -1;
	} else {

	    // Toggle bits only change on an OE edge.
	    if (flashmem.prev_oe)
		flash_update_status_byte();

	    pins->data_drv = 1;
	    pins->data_out = flashmem.busy ? flashmem.status_byte : flashmem.data[addr];

	    if (addr != flashmem.cycle_prev_addr) {
		flashmem.cycle_count++;
		flashmem.cycle_prev_addr = addr;
	    }
	}
    }

    flashmem.prev_we = pins->we;
    flashmem.prev_oe = pins->oe;
}

uint32_t flash_size(void)
{
    return sizeof flashmem.data;
}

uint8_t *flash_buffer(void)
{
    return flashmem.data;
}

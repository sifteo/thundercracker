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
    uint16_t addr;
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
} flash;


void flash_init(const char *filename)
{
    size_t result;

    flash.file = fopen(filename, "rb+");
    if (!flash.file) {
        flash.file = fopen(filename, "wb+");
    }
    if (!flash.file) {
	perror("Error opening flash");
	exit(1);
    }

    result = fread(flash.data, 1, FLASH_SIZE, flash.file);
    if (result < 0) {
	perror("Error reading flash");
	exit(1);
    }	
}

static void flash_write(void)
{
    size_t result;
    
    fseek(flash.file, 0, SEEK_SET);

    result = fwrite(flash.data, FLASH_SIZE, 1, flash.file);
    if (result != 1)
	perror("Error writing flash");
}

void flash_exit(void)
{
    if (flash.write_timer)
	flash_write();
    fclose(flash.file);
}

uint32_t flash_cycle_count(void)
{
    uint32_t c = flash.cycle_count;
    flash.cycle_count = 0;
    return c;
}

enum busy_flag flash_busy_flag(void)
{
    // These busy flags are only reset after they're read.
    enum busy_flag f = flash.busy_status;
    flash.busy_status = 0;
    return f;
}

static int flash_match_command(struct command_sequence *seq)
{
    unsigned i;
    uint8_t fifo_index = flash.cmd_fifo_head - CMD_LENGTH + 1;
   
    for (i = 0; i < CMD_LENGTH; i++, fifo_index++) {
	fifo_index &= CMD_FIFO_MASK;

	if ( (flash.cmd_fifo[fifo_index].addr & seq[i].addr_mask) != seq[i].addr ||
	     (flash.cmd_fifo[fifo_index].data & seq[i].data_mask) != seq[i].data )
	    return 0;
    }

    return 1;
}

static void flash_match_commands(void)
{
    struct cmd_state *st = &flash.cmd_fifo[flash.cmd_fifo_head];

    if (flash.busy != BF_IDLE)
	return;

    if (flash_match_command(cmd_byte_program)) {
	flash.data[st->addr] &= st->data;
	flash.status_byte = STATUS_DATA_INV & ~st->data;
	flash.busy = BF_PROGRAM;
	flash.write_timer = USEC_TO_CYCLES(FLUSH_TIME_US);
	flash.busy_timer = USEC_TO_CYCLES(PROGRAM_TIME_US);
	
    } else if (flash_match_command(cmd_sector_erase)) {
	memset(flash.data + (st->addr & ~(SECTOR_SIZE - 1)), 0xFF, SECTOR_SIZE);
	flash.busy = BF_ERASE;
	flash.write_timer = USEC_TO_CYCLES(FLUSH_TIME_US);
	flash.busy_timer = USEC_TO_CYCLES(ERASE_SECTOR_TIME_US);
	
    } else if (flash_match_command(cmd_block_erase)) {
	memset(flash.data + (st->addr & ~(BLOCK_SIZE - 1)), 0xFF, BLOCK_SIZE);
	flash.busy = BF_ERASE;
	flash.write_timer = USEC_TO_CYCLES(FLUSH_TIME_US);
	flash.busy_timer = USEC_TO_CYCLES(ERASE_BLOCK_TIME_US);

    } else if (flash_match_command(cmd_chip_erase)) {
	memset(flash.data, 0xFF, FLASH_SIZE);
	flash.busy = BF_ERASE;
	flash.write_timer = USEC_TO_CYCLES(FLUSH_TIME_US);
	flash.busy_timer = USEC_TO_CYCLES(ERASE_CHIP_TIME_US);
    }
}

static void flash_update_status_byte(void)
{
    flash.status_byte ^= STATUS_TOGGLE;

    if (flash.busy == BF_ERASE)
	flash.status_byte ^= STATUS_ERASE_TOGGLE;
}

void flash_tick(void)
{
    /*
     * March time forward on the current operation, if any.
     */

    if (flash.busy) {
	if (!--flash.busy_timer)
	    flash.busy = BF_IDLE;
    }
    if (flash.write_timer) {
	if (!--flash.write_timer)
	    flash_write();
    }

    // Latch any busy flags long enough for the UI to see them.
    flash.busy_status |= flash.busy;
}

void flash_cycle(struct flash_pins *pins)
{
    if (pins->ce) {
	// Chip disabled
	pins->data_drv = 0;
	flash.cycle_prev_addr = -1;
	flash.prev_we = 1;

    } else {
	uint32_t addr = (FLASH_SIZE - 1) & pins->addr;

	/*
	 * Command writes are triggered on a falling WE edge.
	 */

	if (!pins->we && flash.prev_we) {
	    flash.cmd_fifo[flash.cmd_fifo_head].addr = addr;
	    flash.cmd_fifo[flash.cmd_fifo_head].data = pins->data_in;
	    flash_match_commands();
	    flash.cmd_fifo_head = CMD_FIFO_MASK & (flash.cmd_fifo_head + 1);
	    flash.cycle_count++;
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
	    flash.cycle_prev_addr = -1;
	} else {

	    // Toggle bits only change on an OE edge.
	    if (flash.prev_oe)
		flash_update_status_byte();

	    pins->data_drv = 1;
	    pins->data_out = flash.busy ? flash.status_byte : flash.data[addr];

	    if (addr != flash.cycle_prev_addr) {
		flash.cycle_count++;
		flash.cycle_prev_addr = addr;
	    }
	}
    }

    flash.prev_we = pins->we;
    flash.prev_oe = pins->oe;
}

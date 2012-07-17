BIN = cube.hex

# Main segment
OBJS = \
        src/main.rel \
        src/graphics_dispatch.rel \
        src/graphics_bg0.rel \
        src/graphics_bg1_setup.rel \
        src/graphics_sprite_setup.rel \
        src/graphics_sprite_line.rel \
        src/graphics_bg2.rel \
        src/graphics_fb32.rel \
        src/graphics_fb64.rel \
        src/graphics_fb128.rel \
        src/graphics_solid.rel \
        src/graphics_stamp.rel \
        src/sensors_init.rel \
        src/sensors_i2c.rel \
        src/sensors_tf0.rel \
        src/sensors_tf1.rel \
        src/sensors_tf2.rel \
        src/overlay.rel \
        src/power.rel \
        src/lcd.rel \
        src/radio_init.rel \
        src/radio_isr.rel \
        src/flash_hal.rel \
        src/flash_decoder.rel \
        src/params.rel \
        src/draw.rel \
        src/tilerom.rel \
        src/disconnected.rel

# Funny BG1 segment, to support ACALL alignment. Order matters.
# bg1_line needs to be at the beginning of the segment; bg0_rom
# is just the right size to pad the rest of the segment up until
# the tilerom data begins.
OBJS += \
        src/graphics_bg1_line.rel \
        src/graphics_bg0_rom.rel \

TC_DIR = ../..
FIRMWARE_INC = ../include
SDK_DIR = $(TC_DIR)/sdk

SDCC = sdcc
CDEPS = src/*.h $(FIRMWARE_INC)/*.h $(SDK_DIR)/include/sifteo/*.h
override CFLAGS += -I$(FIRMWARE_INC) -I$(SDK_DIR)/include
override CFLAGS += -DNOT_USERSPACE

# Define our CPU's memory limits
LDFLAGS += --xram-size 1024 --code-size 16384

# All our fast code is in assmebly; we want the C code to be small.
override CFLAGS +=  --opt-code-size

# Place the 2Kb ACALL segment for BG1
LDFLAGS += -Wl-bBG1LINE=0x2000

# Don't use any standard library code! (Size optimization)
LDFLAGS += --nostdlib

# Simple size profiler, based on the .rst parser used by our emulator's SBT.
# This is the default build target; this runs static analysis checks which
# should be performed on every automated build.
sizeprof: $(BIN)
	python $(TC_DIR)/emulator/resources/firmware-sizeprof.py src/*.rst

$(BIN): $(OBJS)
	$(SDCC) -o $@ $(LDFLAGS) $(OBJS)

%.rel: %.c $(CDEPS)
	$(SDCC) -c -o $@ $< $(CFLAGS)

# The tilerom outputs are checked into version control, but this
# rule provides an easy way to regenerate them when necessary.
src/tilerom.c: tilerom/src-palettes.png tilerom/src-tiles.png tilerom/romgen.py
	python tilerom/romgen.py

debug: $(BIN)
	$(SDK_DIR)/bin/siftulator -l $(TC_DIR)/extras/loopy/loopy.elf -n 1 -d -f $(BIN)

tests: $(BIN)
	cd $(TC_DIR)/test/firmware/cube; ../../../sdk/bin/siftulator --headless -e tests.lua -l mc-stub.elf  -f ../../../firmware/cube/cube.hex

cube.bin: cube.hex
	makebin -p $< $@

cube.png: cube.bin
	(echo "P4 256 512"; cat $<) | convert - $@

visualize: cube.png
	open $<

clean:
	rm -f *.mem src/*.rel src/*.rst src/*.sym *.lnk src/*.lst *.map src/*.asm
	rm -f cube.bin cube.png cube.hex
    
.PHONY: clean sizeprof debug tests visualize

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
        src/overlay.rel \
        src/power.rel \
        src/lcd.rel \
        src/radio.rel \
        src/sensors.rel \
        src/battery.rel \
        src/flash_hal.rel \
        src/flash_decoder.rel \
        src/params.rel \
        src/draw.rel \
        src/tilerom.rel \
        src/demo.rel \
        
# Funny BG1 segment, to support ACALL alignment. Order matters.
# bg1_line needs to be at the beginning of the segment; bg0_rom
# is just the right size to pad the rest of the segment up until
# the tilerom data begins.
OBJS += \
        src/graphics_bg1_line.rel \
        src/graphics_bg0_rom.rel \
        
FIRMWARE_INC = ../include
SDK_DIR = ../../sdk

SDCC = sdcc
CDEPS = src/*.h $(FIRMWARE_INC)/*.h $(SDK_DIR)/include/sifteo/*.h
CFLAGS += -I$(FIRMWARE_INC) -I$(SDK_DIR)/include
CFLAGS += -DNOT_USERSPACE

# Define our CPU's memory limits
LDFLAGS += --xram-size 1024 --code-size 16384

# All our fast code is in assmebly; we want the C code to be small.
CFLAGS +=  --opt-code-size

# Place the 2Kb ACALL segment for BG1
LDFLAGS += -Wl-bBG1LINE=0x2000

$(BIN): $(OBJS)
	$(SDCC) -o $@ $(LDFLAGS) $(OBJS)

%.rel: %.c $(CDEPS)
	$(SDCC) -c -o $@ $< $(CFLAGS)

# The tilerom outputs are checked into version control, but this
# rule provides an easy way to regenerate them when necessary.
src/tilerom.c: tilerom/src-palettes.png tilerom/src-tiles.png tilerom/romgen.py
	python tilerom/romgen.py

clean:
	rm -f *.mem src/*.rel src/*.rst src/*.sym *.lnk src/*.lst *.map src/*.asm

# Simple size profiler, based on the .rst parser used by our emulator's SBT
sizeprof: $(BIN)
	python ../../emulator/resources/firmware-sizeprof.py src/*.rst
    
.PHONY: clean sizeprof

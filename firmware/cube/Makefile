BIN = cube.ihx

# Main segment
OBJS = \
        main.rel \
        graphics_dispatch.rel \
        graphics_bg0.rel \
        graphics_bg1_setup.rel \
        graphics_sprite_setup.rel \
        graphics_sprite_line.rel \
        graphics_bg2.rel \
        graphics_fb32.rel \
        graphics_fb64.rel \
        graphics_fb128.rel \
        graphics_solid.rel \
        lcd.rel \
        radio.rel \
        sensors.rel \
        flash_hal.rel \
        flash_decoder.rel \
        draw.rel \
        tilerom.rel \
        demo.rel \
	
# Funny BG1 segment, to support ACALL alignment. Order matters.
# bg1_line needs to be at the beginning of the segment; bg0_rom
# is just the right size to pad the rest of the segment up until
# the tilerom data begins.
OBJS += \
	graphics_bg1_line.rel \
        graphics_bg0_rom.rel \
        
FIRMWARE_INC = ../include
SDK_DIR = ../../sdk

SDCC = sdcc
CDEPS = *.h $(FIRMWARE_INC)/*.h $(SDK_DIR)/include/sifteo/*.h
CFLAGS += -I$(FIRMWARE_INC) -I$(SDK_DIR)/include

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
tilerom.c: tilerom/src-palettes.png tilerom/src-tiles.png tilerom/romgen.py
	python tilerom/romgen.py

clean:
	rm -f *.mem *.rel *.rst *.sym *.lnk *.lst *.map *.asm

# Simple size profiler, based on the .rst parser used by our emulator's SBT
sizeprof: $(BIN)
	python ../../emulator/resources/firmware-sizeprof.py *.rst
    
.PHONY: clean sizeprof
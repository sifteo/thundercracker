BIN = cube.ihx

OBJS = \
        main.rel \
        graphics_dispatch.rel \
        graphics_bg0.rel \
        graphics_bg1.rel \
        graphics_sprite.rel \
        graphics_bg0_rom.rel \
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
        time.rel \
        draw.rel \
        tilerom.rel \
        demo.rel \

FIRMWARE_INC = ../include
SDK_DIR = ../../sdk

SDCC = sdcc
CDEPS = *.h $(FIRMWARE_INC)/*.h $(SDK_DIR)/include/sifteo/*.h
CFLAGS += -I$(FIRMWARE_INC) -I$(SDK_DIR)/include
LDFLAGS += --xram-size 1024 --code-size 16384

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
	python ../../emulator/scripts/firmware-sizeprof.py *.rst
    
.PHONY: clean sizeprof
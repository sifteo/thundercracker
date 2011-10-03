BIN = cube.ihx

OBJS = \
	graphics.rel \
	lcd.rel \
	radio.rel \
	sensors.rel \
	flash_hal.rel \
	flash_decoder.rel \
	main.rel \
	time.rel \
	draw.rel \
	tilerom.rel \
	demo.rel \

FIRMWARE_INC = ../include
SDK_DIR = ../../sdk

SDCC = sdcc
CDEPS = *.h $(FIRMWARE_INC)/*.h $(SDK_DIR)/include/sifteo/*.h
CFLAGS = -I$(FIRMWARE_INC) -I$(SDK_DIR)/include
LDFLAGS = --xram-size 1024 --code-size 16384

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

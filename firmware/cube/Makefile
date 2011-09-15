BIN = cube.ihx

OBJS = \
	graphics.rel \
	radio.rel \
	sensors.rel \
	flash_hal.rel \
	flash_decoder.rel \
	main.rel \

FIRMWARE_INC = ../include
CDEPS = *.h $(FIRMWARE_INC)/*.h

SDCC = sdcc
CFLAGS = -I$(FIRMWARE_INC)

$(BIN): $(OBJS)
	$(SDCC) -o $@ $(OBJS)

%.rel: %.c $(CDEPS)
	$(SDCC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f *.mem *.rel *.rst *.sym *.lnk *.lst *.map *.asm

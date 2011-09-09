all: spi-test.hex

CDEPS = *.h
SDCC = sdcc

spi-test.hex: spi-test.c $(CDEPS)
	$(SDCC) $< $(CFLAGS)
	mv spi-test.ihx spi-test.hex

clean:
	rm -f *.hex *.ihx *.mem *.rel *.rst *.sym *.lnk *.lst *.map *.asm

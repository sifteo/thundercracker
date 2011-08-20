all: demo.ihx spi-test.ihx

CDEPS = *.h
SDCC = sdcc

demo.ihx: demo.c $(CDEPS)
	$(SDCC) $<

spi-test.ihx: spi-test.c $(CDEPS)
	$(SDCC) $<

clean:
	rm -f *.ihx *.mem *.rel *.rst *.sym *.lnk *.lst *.map *.asm

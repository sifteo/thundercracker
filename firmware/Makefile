all: demo.ihx spi-test.ihx

CDEPS = *.h
CC = sdcc

demo.ihx: demo.c $(CDEPS)
	$(CC) $<

spi-test.ihx: spi-test.c $(CDEPS)
	$(CC) $<

clean:
	rm -f *.ihx *.mem *.rel *.rst *.sym *.lnk *.lst *.map *.asm

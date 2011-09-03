all: demo.ihx tear.ihx flash.bin

CDEPS = *.h
SDCC = sdcc
CFLAGS = -I../../firmware/cube

demo.ihx: demo.c $(CDEPS)
	$(SDCC) $< $(CFLAGS)

tear.ihx: tear.c $(CDEPS)
	$(SDCC) $< $(CFLAGS)

clean:
	rm -f flash.bin *.ihx *.mem *.rel *.rst *.sym *.lnk *.lst *.map *.asm

PY_PATH := /opt/local/bin:/usr/bin
PYTHON  := python2.7

flash.bin: assets/flashgen.py assets/*.png
	PATH=$(PY_PATH) $(PYTHON) $<

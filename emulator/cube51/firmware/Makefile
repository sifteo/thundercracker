all: demo.ihx radiogfx.ihx tear.ihx

CDEPS = *.h
SDCC = sdcc

demo.ihx: demo.c $(CDEPS)
	$(SDCC) $<

radiogfx.ihx: radiogfx.c $(CDEPS)
	$(SDCC) $<

tear.ihx: tear.c $(CDEPS)
	$(SDCC) $<

clean:
	rm -f *.ihx *.mem *.rel *.rst *.sym *.lnk *.lst *.map *.asm

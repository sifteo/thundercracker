BIN = ../cube.ihx

OBJS = \
	radiogfx.rel \

CDEPS = *.h
SDCC = sdcc
CFLAGS =

$(BIN): $(OBJS)
	$(SDCC) -o $@ $(OBJS)

%.rel: %.c $(CDEPS)
	$(SDCC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f *.mem *.rel *.rst *.sym *.lnk *.lst *.map *.asm

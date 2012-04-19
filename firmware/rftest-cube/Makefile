BIN = rftest-cube.hex
OBJS = main.rel
        
FIRMWARE_INC = ../include
SDK_DIR = ../../sdk

SDCC = sdcc
CDEPS = $(FIRMWARE_INC)/*.h $(SDK_DIR)/include/sifteo/*.h
CFLAGS += -I$(FIRMWARE_INC) -I$(SDK_DIR)/include
CFLAGS += -DNOT_USERSPACE

# Define our CPU's memory limits
LDFLAGS += --xram-size 1024 --code-size 16384

$(BIN): $(OBJS)
	$(SDCC) -o $@ $(LDFLAGS) $(OBJS)

%.rel: %.c $(CDEPS)
	$(SDCC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f *.mem *.rel *.rst *.sym *.lnk *.lst *.map *.asm

.PHONY: clean

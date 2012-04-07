
BIN = testjig.elf

TC_DIR = ../../..

include $(TC_DIR)/firmware/master/Makefile.objs
include $(TC_DIR)/Makefile.platform

# Test Jig specific sources
OBJS_STM32 +=   $(TC_DIR)/firmware/master/stm32/neighbor.stm32.o

# CFLAGS gets added to the platform specific CFLAGS within Makefile.defs
CFLAGS := -I../common -I../../include -DNO_USERSPACE_HEADERS
CFLAGS_STM32 := -I../stm32 -I../testjig

include ../Makefile.defs

# Override default CDEPS
CDEPS := ../common/syscall-table.def

all: $(BIN)

$(BIN): $(OBJS_STM32)
	@echo
	$(CC_STM32) -o $@ $^ $(LDFLAGS_STM32)

%.stm32.o: %.cpp $(CDEPS)
	@echo
	$(CC_STM32) -c $(CCFLAGS_STM32) $(DEPFLAGS) $< -o $@

%.stm32.o: %.c $(CDEPS)
	@echo
	$(CC_STM32) -c $(CFLAGS_STM32) $(DEPFLAGS) $< -o $@

common/syscall-table.def: $(TC_DIR)/sdk/include/sifteo/abi.h
	python ../tools/firmware-syscall-table.py < $< > $@

ocd:
	openocd -f $(MASTER_DIR)/stm32/$(PROG_CONFIG) \
		-c init \
		-c "reset halt"

unlock:
	openocd -f $(MASTER_DIR)/stm32/$(PROG_CONFIG) \
		-c init \
		-c "reset halt" \
		-c "stm32f1x unlock 0" \
		-c reset \
		-c exit

program: $(BIN_STM32)
	openocd -f $(MASTER_DIR)/stm32/$(PROG_CONFIG) \
		-c init \
		-c "reset halt" \
		-c "flash write_image erase $<" \
		-c "verify_image $<" \
		-c "reset run" \
		-c exit

.PHONY: all clean syscall-table ocd unlock program

# Several steps, to work around Win32 command line length limits
clean:
	rm -f $(BINS)
	rm -f $(OBJS_SIM)
	rm -f $(OBJS_STM32) 
	rm -rf .dep

# include the dep files, and make a folder for them if necessary
include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

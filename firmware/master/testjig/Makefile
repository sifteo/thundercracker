BIN = testjig.elf
TC_DIR = ../../..

include $(TC_DIR)/firmware/master/Makefile.objs
include $(TC_DIR)/firmware/master/Makefile.defs

# testjig specific flags
CCFLAGS += -I. -DBOARD=BOARD_TEST_JIG

# Override default CDEPS
CDEPS := ../common/syscall-table.def

all: $(BIN)

$(BIN): $(OBJS_STM32_TESTJIG)
	$(CC) -o $@ $^ $(LDFLAGS)

%.stm32.o: %.cpp $(CDEPS)
	$(CC) -c $(CCFLAGS) $(DEPFLAGS) $< -o $@

../common/syscall-table.def: $(TC_DIR)/sdk/include/sifteo/abi.h
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

program: $(BIN)
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
	rm -f $(BIN)
	rm -f $(OBJS_STM32_TESTJIG)
	rm -rf .dep

# include the dep files, and make a folder for them if necessary
include $(shell mkdir .dep 2>/dev/null) $(wildcard .dep/*)

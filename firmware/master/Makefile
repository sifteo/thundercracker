BIN_SIM = master-sim
BIN_STM32 = master-stm32.elf

BINS = $(BIN_SIM)
ifneq ($(BUILD_STM32),)
	BINS += $(BIN_STM32)
endif

TC_DIR = ../..

include Makefile.objs
include $(TC_DIR)/Makefile.platform

# STM32 only dependencies
SPEEX_DIR = speex
include $(SPEEX_DIR)/speex.mk
OBJS_STM32 += $(SPEEX_OBJS_STM32)

# CFLAGS gets added to the platform specific CFLAGS within Makefile.defs
CFLAGS := -Icommon -I../include -DNO_USERSPACE_HEADERS
CFLAGS_STM32 := -Istm32 $(SPEEX_INC) -DHAVE_CONFIG_H -D_BUILD_SPEEX -DDISABLE_ENCODER -DSTRIP_ENCODER
CFLAGS_SIM = -I$(DEPS_DIR)/portaudio/include \
             -I$(DEPS_DIR)/speex/include

#Determines whether USB blocks the execution of the master firmware
#in order to load assets. 
ifeq ($(USB_LOAD), 1)
	CFLAGS_STM32 += -DUSB_LOAD
endif

ifneq ($(MASTER_RF_CHAN),)
    CFLAGS += -DMASTER_RF_CHAN=$(MASTER_RF_CHAN)
endif

include Makefile.defs

# Override default CDEPS
CDEPS := \
	common/syscall-table.def \
	common/*.h \
	stm32/*.h \
	../include/*.h \
	$(SDK_DIR)/include/sifteo/*.h \
    $(SPEEX_DEPS)

all: $(BINS)

$(BIN_SIM): $(OBJS_SIM)
	$(CC_SIM) -o $@ $^ $(LDFLAGS_SIM)

$(BIN_STM32): $(OBJS_STM32)
	$(CC_STM32) -o $@ $^ $(LDFLAGS_STM32)

%.sim.o: %.cpp $(CDEPS)
	$(CC_SIM) -c -o $@ $< $(CCFLAGS_SIM)

%.stm32.o: %.cpp $(CDEPS)
	$(CC_STM32) -c -o $@ $< $(CCFLAGS_STM32)

%.stm32.o: %.c $(CDEPS)
	$(CC_STM32) -c -o $@ $< $(CFLAGS_STM32)

%.stm32.o: %.s $(CDEPS)
	$(CC_STM32) -x assembler-with-cpp -c -o $@ $< $(CFLAGS_STM32)

common/syscall-table.def: $(TC_DIR)/sdk/include/sifteo/abi.h
	python tools/firmware-syscall-table.py < $< > $@

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

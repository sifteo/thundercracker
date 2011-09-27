BIN_SIM = master-sim.a
BIN_STM32 = master-stm32.a
BINS = $(BIN_SIM) $(BIN_STM32)

OBJS_SIM = \
	common/syscall.sim.o \
	common/runtime.sim.o \
	common/radio.sim.o \
	common/cube.sim.o \
	common/cubecodec.sim.o \
	sim/radio_nethub.sim.o \
	sim/main.sim.o \

OBJS_STM32 = \
	common/syscall.stm32.o \
	common/runtime.stm32.o \
	common/radio.stm32.o \
	common/cube.stm32.o \
	common/cubecodec.stm32.o \
	stm32/setup.stm32.o \
	stm32/radio.stm32.o \

SDK_DIR = ../../sdk
FIRMWARE_INC = ../include

CDEPS = common/*.h $(FIRMWARE_INC)/*.h $(SDK_DIR)/include/sifteo/*.h
CCFLAGS += -I$(FIRMWARE_INC) -Icommon

include Makefile.rules

all: $(BINS)

$(BIN_SIM): $(OBJS_SIM)
	rm -f $@
	$(AR) r $@ $^

$(BIN_STM32): $(OBJS_STM32)
	rm -f $@
	$(AR) r $@ $^

.PHONY: all clean

clean:
	rm -f $(BINS) $(OBJS_SIM) $(OBJS_STM32)

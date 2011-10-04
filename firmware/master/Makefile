BIN_SIM = master-sim.a
BIN_STM32 = master-stm32.a
BINS = $(BIN_SIM) $(BIN_STM32)

SPEEX_DIR = speex
include $(SPEEX_DIR)/speex.mk

OBJS_SIM = \
	common/syscall.sim.o \
	common/runtime.sim.o \
	common/radio.sim.o \
	common/cube.sim.o \
	common/cubecodec.sim.o \
	sim/radio_nethub.sim.o \
	sim/systime.sim.o \
	sim/main.sim.o \

OBJS_STM32 = \
	common/syscall.stm32.o \
	common/runtime.stm32.o \
	common/radio.stm32.o \
	common/cube.stm32.o \
	common/cubecodec.stm32.o \
	stm32/systime.stm32.o \
	stm32/vectors.stm32.o \
	stm32/setup.stm32.o \
	stm32/radio.stm32.o \
	stm32/gpio.stm32.o \
	stm32/spi.stm32.o \
	stm32/nrf24l01.stm32.o \
	stm32/debug.stm32.o \
    $(SPEEX_OBJS_STM32) \

TC_DIR = ../..
FIRMWARE_INC := ../include
CCFLAGS += -I$(FIRMWARE_INC) -Icommon $(SPEEX_INC) -DHAVE_CONFIG_H -D_BUILD_SPEEX

include Makefile.defs

# Override default CDEPS
CDEPS := \
	common/*.h \
	stm32/*.h \
	$(FIRMWARE_INC)/*.h \
	$(SDK_DIR)/include/sifteo/*.h \
    $(SPEEX_DEPS) \

all: $(BINS)

$(BIN_SIM): $(OBJS_SIM)
	rm -f $@
	$(AR) r $@ $^
	$(RANLIB_SIM) $@

$(BIN_STM32): $(OBJS_STM32)
	rm -f $@
	$(AR) r $@ $^
	$(RANLIB_STM32) $@

%.sim.o: %.cpp $(CDEPS)
	$(CC_SIM) -c -o $@ $< $(CCFLAGS_SIM)

%.stm32.o: %.cpp $(CDEPS)
	$(CC_STM32) -c -o $@ $< $(CCFLAGS_STM32)

%.stm32.o: %.c $(CDEPS)
	$(CC_STM32) -c -o $@ $< $(CCFLAGS_STM32)

%.stm32.o: %.s $(CDEPS)
	$(CC_STM32) -x assembler-with-cpp -c -o $@ $< $(CCFLAGS_STM32)

.PHONY: all clean

clean:
	rm -f $(BINS) $(OBJS_SIM) $(OBJS_STM32)

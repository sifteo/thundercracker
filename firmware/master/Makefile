BIN_SIM = master-sim.a
BIN_STM32 = master-stm32.a

BINS = $(BIN_SIM)
ifneq ($(BUILD_STM32),)
	BINS += $(BIN_STM32)
endif

SPEEX_DIR = speex
include $(SPEEX_DIR)/speex.mk

OBJS_SIM = \
	common/syscall.sim.o \
	common/runtime.sim.o \
	common/radio.sim.o \
	common/cube.sim.o \
	common/cubecodec.sim.o \
	common/audiomixer.sim.o \
	common/speexdecoder.sim.o \
	common/audiobuffer.sim.o \
	common/audiochannel.sim.o \
	common/accel.sim.o \
	common/neighbors.sim.o \
	sim/audiooutdevice.sim.o \
	sim/portaudiooutdevice.sim.o \
	sim/flashlayer.sim.o \
	sim/siftulator.sim.o \
	sim/main.sim.o

OBJS_STM32 = \
	common/syscall.stm32.o \
	common/runtime.stm32.o \
	common/radio.stm32.o \
	common/cube.stm32.o \
	common/cubecodec.stm32.o \
	common/audiomixer.stm32.o \
	common/speexdecoder.stm32.o \
	common/audio.gen.stm32.o \
	common/audiobuffer.stm32.o \
	common/audiochannel.stm32.o \
	common/accel.stm32.o \
	common/neighbors.stm32.o \
	stm32/systime.stm32.o \
	stm32/vectors.stm32.o \
	stm32/setup.stm32.o \
	stm32/radio.stm32.o \
	stm32/gpio.stm32.o \
	stm32/spi.stm32.o \
	stm32/nrf24l01.stm32.o \
	stm32/debug.stm32.o \
    stm32/usart.stm32.o \
    stm32/dma.stm32.o \
    stm32/button.stm32.o \
    stm32/hwtimer.stm32.o \
    stm32/pwmaudioout.stm32.o \
    stm32/audiooutdevice.stm32.o \
    stm32/flashlayer.stm32.o \
    $(SPEEX_OBJS_STM32) \

include Makefile.platform

TC_DIR = ../..
ifeq ($(BUILD_PLATFORM), windows32)
    DEPS_DIR = $(TC_DIR)/deps/win32
    # TBD if there's a better way, but windows ar or libtool needs to have all the
    # individual object files available from any static libs you want to merge
    # in. these have been extracted in the deps folder via `ar -x libfoo.a`
    SIM_STATIC_OBJS = $(DEPS_DIR)/speex/lib/obj/*.o \
                        $(DEPS_DIR)/portaudio/lib/obj/*.o
else
    DEPS_DIR = $(TC_DIR)/deps/osx
    SIM_STATIC_LIBS = $(DEPS_DIR)/speex/lib/libspeex.a \
                        $(DEPS_DIR)/portaudio/lib/libportaudio.a
endif

# CFLAGS gets added to the platform specific CFLAGS within Makefile.defs
CFLAGS := -Icommon -I../include
CFLAGS_STM32 := $(SPEEX_INC) -DHAVE_CONFIG_H -D_BUILD_SPEEX -DDISABLE_ENCODER
CFLAGS_SIM = -I$(DEPS_DIR)/portaudio/include \
             -I$(DEPS_DIR)/speex/include

include Makefile.defs

# Override default CDEPS
CDEPS := \
	common/*.h \
	stm32/*.h \
	../include/*.h \
	$(SDK_DIR)/include/sifteo/*.h \
    $(SPEEX_DEPS) \

all: $(BINS)

ifeq ($(BUILD_PLATFORM), windows32)
$(BIN_SIM): $(OBJS_SIM)
	rm -f $@
	$(AR) rcs $@ $^ $(SIM_STATIC_OBJS)
else
$(BIN_SIM): $(OBJS_SIM)
	rm -f $@
	$(LIBTOOL_SIM) -static -o $@ $^ $(SIM_STATIC_LIBS)
endif

$(BIN_STM32): $(OBJS_STM32)
	rm -f $@
	$(AR) r $@ $^
	$(RANLIB_STM32) $@

%.sim.o: %.cpp $(CDEPS)
	$(CC_SIM) -c -o $@ $< $(CCFLAGS_SIM)

%.stm32.o: %.cpp $(CDEPS)
	$(CC_STM32) -c -o $@ $< $(CCFLAGS_STM32)

%.stm32.o: %.c $(CDEPS)
	$(CC_STM32) -c -o $@ $< $(CFLAGS_STM32)

%.stm32.o: %.s $(CDEPS)
	$(CC_STM32) -x assembler-with-cpp -c -o $@ $< $(CFLAGS_STM32)

.PHONY: all clean

clean:
	rm -f $(BINS) $(OBJS_SIM) $(OBJS_STM32)

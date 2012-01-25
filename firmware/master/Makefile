BIN_SIM = master-sim.a
BIN_STM32 = master-stm32.a

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
include stm32/st-usb/st-usb.mk
OBJS_STM32 +=   $(OBJS_USB_STM32) \
                $(SPEEX_OBJS_STM32)

ifeq ($(BUILD_PLATFORM), windows32)
	# TBD if there's a better way, but windows ar or libtool needs to have all the
	# individual object files available from any static libs you want to merge
	# in. these have been extracted in the deps folder via `ar -x libfoo.a`
	SIM_STATIC_OBJS = $(DEPS_DIR)/speex/lib/obj/*.o $(DEPS_DIR)/portaudio/lib/obj/*.o
else
	SIM_STATIC_LIBS = \
		$(DEPS_DIR)/speex/lib/libspeex.a \
		$(DEPS_DIR)/portaudio/lib/libportaudio.a
endif

# CFLAGS gets added to the platform specific CFLAGS within Makefile.defs
CFLAGS := -Icommon -I../include
CFLAGS_STM32 := -Istm32 $(SPEEX_INC) $(INC_USB_STM32) -DHAVE_CONFIG_H -D_BUILD_SPEEX -DDISABLE_ENCODER -DSTRIP_ENCODER
# NOTE: ST defines here are temporary until we integrate our own USB code
CFLAGS_STM32 += -DUSE_STM3210C_EVAL -DSTM32F10X_CL -DUSE_STDPERIPH_DRIVER
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
	$(SDK_DIR)/include/sifteo/*.h $(STIR)

all: $(BINS)

ifeq ($(BUILD_PLATFORM), Darwin)
$(BIN_SIM): $(OBJS_SIM)
	rm -f $@
	$(LIBTOOL_SIM) -static -o $@ $^ $(SIM_STATIC_LIBS)
else
$(BIN_SIM): $(OBJS_SIM)
	rm -f $@
	$(AR) rcs $@ $^ $(SIM_STATIC_OBJS)
endif

$(BIN_STM32): $(OBJS_STM32)
	rm -f $@
	$(AR_STM32) r $@ $^
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

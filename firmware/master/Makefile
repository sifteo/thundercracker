BIN_SIM = master-sim.a
BIN_STM32 = master-stm32.a

BINS = $(BIN_SIM)
ifneq ($(BUILD_STM32),)
	BINS += $(BIN_STM32)
endif

SPEEX_DIR = speex
include $(SPEEX_DIR)/speex.mk

SELECTORMENU_GEN_FILES = $(MASTER_DIR)/selectormenu/$(ASSETS).gen.h $(MASTER_DIR)/selectormenu/$(ASSETS).gen.cpp $(MASTER_DIR)/selectormenu/$(ASSETS).html $(MASTER_DIR)/selectormenu/$(AUDIO).gen.h $(MASTER_DIR)/selectormenu/$(AUDIO).gen.cpp

include Makefile.objs
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
	$(SDK_DIR)/include/sifteo/*.h $(STIR)

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

ASSETDEPS += $(MASTER_DIR)/selectormenu/*.png $(MASTER_DIR)/selectormenu/$(ASSETS).lua
AUDIODEPS += $(MASTER_DIR)/selectormenu/*.raw $(MASTER_DIR)/selectormenu/$(AUDIO).txt

$(MASTER_DIR)/selectormenu/$(ASSETS).gen.cpp: $(STIR) $(ASSETDEPS)
	$(STIR) $(MASTER_DIR)/selectormenu/$(ASSETS).lua -o $(MASTER_DIR)/selectormenu/$(ASSETS).gen.cpp -o $(MASTER_DIR)/selectormenu/$(ASSETS).gen.h -o $(MASTER_DIR)/selectormenu/$(ASSETS).html -v

$(MASTER_DIR)/selectormenu/$(AUDIO).gen.cpp: $(SPEEXENC) $(AUDIODEPS)
	$(SPEEXENC) -o $(MASTER_DIR)/selectormenu/$(AUDIO).gen.cpp -o $(MASTER_DIR)/selectormenu/$(AUDIO).gen.h $(MASTER_DIR)/selectormenu/$(AUDIO).txt

.PHONY: all clean

clean:
	rm -f $(BINS) $(OBJS_SIM) $(OBJS_STM32) $(SELECTORMENU_GEN_FILES)

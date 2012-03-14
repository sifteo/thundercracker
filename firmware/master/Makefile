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
include stm32/st-usb/st-usb.mk
OBJS_STM32 +=   $(OBJS_USB_STM32) \
                $(SPEEX_OBJS_STM32)

# CFLAGS gets added to the platform specific CFLAGS within Makefile.defs
CFLAGS := -Icommon -I../include -DFW_BUILD
CFLAGS_STM32 := -Istm32 $(SPEEX_INC) $(INC_USB_STM32) -DHAVE_CONFIG_H -D_BUILD_SPEEX -DDISABLE_ENCODER -DSTRIP_ENCODER
# NOTE: ST defines here are temporary until we integrate our own USB code
CFLAGS_STM32 += -DUSE_STM3210C_EVAL -DSTM32F10X_CL -DUSE_STDPERIPH_DRIVER
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

ifneq ($(SVM_TRACE),)
    CFLAGS_SIM += -DSVM_TRACE
endif

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

syscall-table:
	python $(TC_DIR)/vm/tools/syscall-table.py \
	   < $(TC_DIR)/sdk/include/sifteo/abi.h \
	   > $(MASTER_DIR)/common/syscall-table.def

.PHONY: all clean syscall-table

# Several steps, to work around Win32 command line length limits
clean:
	rm -f $(BINS)
	rm -f $(OBJS_SIM)
	rm -f $(OBJS_STM32) 

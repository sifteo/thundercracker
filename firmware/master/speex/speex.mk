
ifeq ($(SPEEX_DIR),)
    $(error Error: SPEEX_DIR must be defined)
endif

SPEEX_OBJS_STM32 = \
    $(SPEEX_DIR)/STM32/libspeex/cb_search.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/filters.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/ltp.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/modes.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/nb_celp.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/quant_lsp.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/vq.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/cb_search.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/gcc/filters_cortexM3.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/gcc/ltp_cortexM3.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/gcc/vq_cortexM3.stm32.o

SPEEX_INC = \
    -I$(SPEEX_DIR)/STM32 \
    -I$(SPEEX_DIR)/STM32/include/speex \
    -I$(SPEEX_DIR)/STM32/libspeex/gcc \
    -I$(SPEEX_DIR)/include/speex \
    -I$(SPEEX_DIR)/libspeex

SPEEX_DEPS = \
    $(SPEEX_DIR)/STM32/*.h \
    $(SPEEX_DIR)/STM32/include/speex/*.h \
    $(SPEEX_DIR)/STM32/libspeex/gcc/*.h \
    $(SPEEX_DIR)/include/speex/*.h \
    $(SPEEX_DIR)/libspeex/*.h

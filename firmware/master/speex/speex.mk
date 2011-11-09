
ifeq ($(SPEEX_DIR),)
    $(error Error: SPEEX_DIR must be defined)
endif

SPEEX_OBJS_STM32 = \
    $(SPEEX_DIR)/libspeex/window.stm32.o \
    $(SPEEX_DIR)/libspeex/bits.stm32.o \
    $(SPEEX_DIR)/libspeex/exc_10_32_table.stm32.o \
    $(SPEEX_DIR)/libspeex/gain_table_lbr.stm32.o \
    $(SPEEX_DIR)/libspeex/lpc.stm32.o \
    $(SPEEX_DIR)/libspeex/lsp.stm32.o \
    $(SPEEX_DIR)/libspeex/speex.stm32.o \
    $(SPEEX_DIR)/libspeex/speex_callbacks.stm32.o \
    $(SPEEX_DIR)/libspeex/speex_header.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/modes.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/nb_celp.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/quant_lsp.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/vq.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/cb_search.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/filters.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/ltp.stm32.o \
    $(SPEEX_DIR)/libspeex/buffer.stm32.o \
    $(SPEEX_DIR)/libspeex/high_lsp_tables.stm32.o \
    $(SPEEX_DIR)/libspeex/lsp_tables_nb.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/gcc/filters_cortexM3.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/gcc/ltp_cortexM3.stm32.o \
    $(SPEEX_DIR)/STM32/libspeex/gcc/vq_cortexM3.stm32.o

SPEEX_INC = \
    -I$(SPEEX_DIR) \
    -I$(SPEEX_DIR)/include \
    -I$(SPEEX_DIR)/STM32 \
    -I$(SPEEX_DIR)/STM32/libspeex/gcc \
    -I$(SPEEX_DIR)/libspeex

SPEEX_DEPS = \
    $(SPEEX_DIR)/include/speex/*.h \
    $(SPEEX_DIR)/libspeex/*.h


ifeq ($(SPEEX_DIR),)
    $(error Error: SPEEX_DIR must be defined)
endif

SPEEX_OBJS_STM32 = \
    $(SPEEX_DIR)/libspeex/window.stm32.o \
    $(SPEEX_DIR)/libspeex/bits.stm32.o \
    $(SPEEX_DIR)/libspeex/exc_10_32_table.stm32.o \
    $(SPEEX_DIR)/libspeex/exc_10_16_table.stm32.o \
    $(SPEEX_DIR)/libspeex/exc_20_32_table.stm32.o \
    $(SPEEX_DIR)/libspeex/exc_5_64_table.stm32.o \
    $(SPEEX_DIR)/libspeex/exc_8_128_table.stm32.o \
    $(SPEEX_DIR)/libspeex/exc_5_256_table.stm32.o \
    $(SPEEX_DIR)/libspeex/hexc_10_32_table.stm32.o \
    $(SPEEX_DIR)/libspeex/hexc_table.stm32.o \
    $(SPEEX_DIR)/libspeex/gain_table_lbr.stm32.o \
    $(SPEEX_DIR)/libspeex/gain_table.stm32.o \
    $(SPEEX_DIR)/libspeex/lpc.stm32.o \
    $(SPEEX_DIR)/libspeex/lsp.stm32.o \
    $(SPEEX_DIR)/libspeex/speex.stm32.o \
    $(SPEEX_DIR)/libspeex/speex_callbacks.stm32.o \
    $(SPEEX_DIR)/libspeex/speex_header.stm32.o \
    $(SPEEX_DIR)/libspeex/modes.stm32.o \
    $(SPEEX_DIR)/libspeex/modes_wb.stm32.o \
    $(SPEEX_DIR)/libspeex/nb_celp.stm32.o \
    $(SPEEX_DIR)/libspeex/sb_celp.stm32.o \
    $(SPEEX_DIR)/libspeex/quant_lsp.stm32.o \
    $(SPEEX_DIR)/libspeex/vq.stm32.o \
    $(SPEEX_DIR)/libspeex/cb_search.stm32.o \
    $(SPEEX_DIR)/libspeex/filters.stm32.o \
    $(SPEEX_DIR)/libspeex/ltp.stm32.o \
    $(SPEEX_DIR)/libspeex/buffer.stm32.o \
    $(SPEEX_DIR)/libspeex/high_lsp_tables.stm32.o \
    $(SPEEX_DIR)/libspeex/lsp_tables_nb.stm32.o

# these are for the narrowband only config, which can be smaller
SPEEX_NB_OBJS_STM32 = \
    $(SPEEX_DIR)/libspeex/window.stm32.o \
    $(SPEEX_DIR)/libspeex/bits.stm32.o \
    $(SPEEX_DIR)/libspeex/exc_10_32_table.stm32.o \
    $(SPEEX_DIR)/libspeex/exc_10_16_table.stm32.o \
    $(SPEEX_DIR)/libspeex/exc_20_32_table.stm32.o \
    $(SPEEX_DIR)/libspeex/exc_5_64_table.stm32.o \
    $(SPEEX_DIR)/libspeex/exc_8_128_table.stm32.o \
    $(SPEEX_DIR)/libspeex/exc_5_256_table.stm32.o \
    $(SPEEX_DIR)/libspeex/gain_table_lbr.stm32.o \
    $(SPEEX_DIR)/libspeex/gain_table.stm32.o \
    $(SPEEX_DIR)/libspeex/lpc.stm32.o \
    $(SPEEX_DIR)/libspeex/lsp.stm32.o \
    $(SPEEX_DIR)/libspeex/speex.stm32.o \
    $(SPEEX_DIR)/libspeex/speex_callbacks.stm32.o \
    $(SPEEX_DIR)/libspeex/speex_header.stm32.o \
    $(SPEEX_DIR)/libspeex/modes.stm32.o \
    $(SPEEX_DIR)/libspeex/nb_celp.stm32.o \
    $(SPEEX_DIR)/libspeex/quant_lsp.stm32.o \
    $(SPEEX_DIR)/libspeex/vq.stm32.o \
    $(SPEEX_DIR)/libspeex/cb_search.stm32.o \
    $(SPEEX_DIR)/libspeex/filters.stm32.o \
    $(SPEEX_DIR)/libspeex/ltp.stm32.o \
    $(SPEEX_DIR)/libspeex/buffer.stm32.o \
    $(SPEEX_DIR)/libspeex/high_lsp_tables.stm32.o \
    $(SPEEX_DIR)/libspeex/lsp_tables_nb.stm32.o

# NOTE - I have had trouble with the cortex-m3 optimized version that ships with ST app note 2812.
# Decoding results in all zeros :( For now, use the C version above from the standard 
# speex download, and come back to resolve this. 
SPEEX_OBJS_CORTEX_M3 = \
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
    -I$(SPEEX_DIR)/STM32/include \
    -I$(SPEEX_DIR)/STM32/libspeex/gcc \
    -I$(SPEEX_DIR)/libspeex

SPEEX_DEPS = \
    $(SPEEX_DIR)/include/speex/*.h \
    $(SPEEX_DIR)/libspeex/*.h \
    $(SPEEX_DIR)/STM32/*.h

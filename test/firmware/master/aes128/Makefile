TC_DIR := ../../../..

BIN := aes128

include $(TC_DIR)/Makefile.platform
include $(TC_DIR)/test/firmware/master/Makefile.defs

OBJS = main.o \
      $(TC_DIR)/firmware/master/common/aes128.o

include $(TC_DIR)/test/firmware/master/Makefile.rules

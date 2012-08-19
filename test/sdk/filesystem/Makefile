APP = test-filesystem

include $(SDK_DIR)/Makefile.defs

OBJS = main.o
TEST_DEPS := *.lua

include $(TC_DIR)/test/sdk/Makefile.rules

SIFTULATOR_FLAGS += -T -n 0
GENERATED_FILES += flash.log flash.snapshot

include $(SDK_DIR)/Makefile.rules

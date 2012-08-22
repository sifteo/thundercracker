APP = test-fault

include $(SDK_DIR)/Makefile.defs

OBJS = main.o memory.o svmvalidator.o
TEST_DEPS := *.lua *.h

include $(TC_DIR)/test/sdk/Makefile.rules

SIFTULATOR_FLAGS += -T -n 0

include $(SDK_DIR)/Makefile.rules

APP = test-assetslot

include $(SDK_DIR)/Makefile.defs

OBJS = $(ASSETS).gen.o main.o
ASSETDEPS += images/*.png $(ASSETS).lua
TEST_DEPS += stub.elf
GENERATED_FILES += stub.elf

include $(TC_DIR)/test/sdk/Makefile.rules

# Load several stub volumes, used to test slot binding
SIFTULATOR_FLAGS += -T stub.elf stub.elf stub.elf stub.elf

stub.elf: stub.o
	$(LD) -o $@ $<

include $(SDK_DIR)/Makefile.rules

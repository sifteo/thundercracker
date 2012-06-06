APP = test-tracker

include $(SDK_DIR)/Makefile.defs

OBJS = $(ASSETS).gen.o main.o
ASSETDEPS += *.xm $(ASSETS).lua

SIFTULATOR_FLAGS = --headless --waveout output.wav -T -n 0
GENERATED_FILES += tests.stamp output.wav

all: tests.stamp

tests.stamp: $(BIN) $(TEST_DEPS)
	@echo "\n================= Running SDK Test:" $(APP) "\n"
	siftulator $(SIFTULATOR_FLAGS) -l $(BIN)
	diff output.wav reference.wav
	echo > $@

.PHONY: all

include $(SDK_DIR)/Makefile.rules

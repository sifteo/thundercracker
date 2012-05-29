#
# This is a special test for Siftulator moreso than for the SDK itself,
# to ensure we can start applications asynchronously.
#
# This only applies on Mac OS, where the normal method of launching ELF
# files involves sending an async message to Siftulator, rather than just
# specifying a game's path on the command line.
#

ifeq ($(shell uname), Darwin)
all: tests.stamp
else
all:
endif

APP = test-async-open

include $(SDK_DIR)/Makefile.defs

OBJS = main.o

GENERATED_FILES += tests.stamp mini-launcher.elf

mini-launcher.elf: mini-launcher.o
	@echo Linking $@ "$(LINK_MESSAGE)"
	@$(LD) -o $@ $(OBJS) $(LDFLAGS)

tests.stamp: $(BIN) mini-launcher.elf
	@echo "\n================= Running SDK Test:" $(APP) "\n"
	rm -f $@
	TEST_STAMP_FILE="$(shell pwd)/tests.stamp" \
		open -W -a $(SDK_DIR)/bin/Siftulator.app $(BIN) \
		--args -l "$(shell pwd)/mini-launcher.elf"
	[ -e $@ ]

.PHONY: all

include $(SDK_DIR)/Makefile.rules

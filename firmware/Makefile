# Default build is determined by environment vars
ifneq ($(BUILD_STM32),)
	TARGETS := master
endif
ifneq ($(BUILD_8051),)
	TARGETS += cube
endif

default: $(TARGETS)

# Explicitly build everything
all: master cube

.PHONY: default all clean cube master

cube:
	@make -C cube

master:
	@make -C master

clean:
	@make -C cube clean
	@make -C master clean

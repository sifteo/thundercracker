TARGETS := master
ifneq ($(BUILD_8051),)
	TARGETS += cube.hex
endif

all: $(TARGETS)

.PHONY: all clean $(TARGETS)

cube.hex:
	@make -C cube
	@cp cube/cube.ihx cube.hex

master:
	@make -C master

clean:
	@make -C cube clean
	@make -C master clean
	rm -f cube.hex

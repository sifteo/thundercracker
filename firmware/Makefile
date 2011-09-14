TARGETS := cube.hex master.a

all: $(TARGETS)

.PHONY: all clean $(TARGETS)

cube.hex:
	@make -C cube
	@cp cube/cube.ihx cube.hex

master.a:
	@make -C master
	@cp master/master.a master.a

clean:
	@make -C cube clean
	@make -C master clean
	rm -f $(TARGETS)

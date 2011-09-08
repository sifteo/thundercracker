BIN = cube.hex

all: $(BIN)

.PHONY: all clean $(BIN)

$(BIN):
	make -C cube
	cp cube/cube.ihx cube.hex

clean:
	make -C cube clean
	rm -f $(BIN)

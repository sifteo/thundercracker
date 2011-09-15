STIR_DIR = ../../stir
CCFLAGS := -I $(STIR_DIR)/src $(STIR_DIR)/src/lodepng.cpp -lstdc++

.PHONY: all clean

all: lsdec

clean:
	rm -f lsdec

lsdec: lsdec.cpp
	$(CC) -o $@ $< $(CCFLAGS)

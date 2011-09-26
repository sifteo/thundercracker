SDK_DIR = ../..
MASTER_LIB = ../../../firmware/master.a
STIR = ../../../stir/stir
ASSETS = assets

BIN = scroller
OBJS = $(ASSETS).gen.o main.o

CDEPS = $(SDK_DIR)/include/sifteo/*.h
ASSETDEPS = $(STIR) $(ASSETS).lua *.png

CCFLAGS += -Os -g -Werror -I$(SDK_DIR)/include
LDFLAGS += -g

$(BIN): $(OBJS) $(MASTER_LIB)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp $(CDEPS) $(ASSETDEPS)
	$(CC) -c -o $@ $< $(CCFLAGS)

$(ASSETS).gen.cpp: $(ASSETDEPS)
	$(STIR) $(ASSETS).lua -o $(ASSETS).gen.cpp -o $(ASSETS).gen.h -o $(ASSETS).html

.PHONY: clean

clean:
	rm -f $(BIN) *.o $(ASSETS).gen.h $(ASSETS).gen.cpp $(ASSETS).html

include $(SDK_DIR)/Makefile.defs

run: tests.stamp

tests.stamp: $(SDK_DIR)/bin/* *.lua mc-stub.elf
	siftulator --headless -e tests.lua -l mc-stub.elf
	echo > $@

mc-stub.elf: mc-stub.o
	slinky -o $@ $<

%.o: %.cpp
	@$(CC) -c -o $@ $< $(CCFLAGS)

clean:
	rm -f tests.stamp trace.txt trace.vcd mc-stub.elf mc-stub.o

.PHONY: run clean

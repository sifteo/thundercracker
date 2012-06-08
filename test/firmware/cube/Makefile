run: tests.stamp

tests.stamp: $(SDK_DIR)/bin/* *.lua
	siftulator --headless -e tests.lua
	echo > $@

clean:
	rm -f tests.stamp trace.txt trace.vcd

.PHONY: run clean

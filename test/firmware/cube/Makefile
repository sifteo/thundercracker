run: tests.stamp

tests.stamp: $(SDK_DIR)/bin/* *.lua
	siftulator -e tests.lua
	echo > $@

clean:
	rm -f tests.stamp

.PHONY: run clean

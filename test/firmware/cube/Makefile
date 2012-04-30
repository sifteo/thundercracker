run: tests.stamp

tests.stamp: $(SDK_DIR)/bin/siftulator *.lua
	TEST= siftulator -e tests.lua
	touch $@

clean:
	rm -f tests.stamp

.PHONY: run clean

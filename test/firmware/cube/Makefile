run: tests.stamp

tests.stamp: $(SDK_DIR)/bin/* *.lua
	TEST= siftulator -e tests.lua
	touch $@

clean:
	rm -f tests.stamp

.PHONY: run clean

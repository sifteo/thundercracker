run: tests.stamp

tests.stamp: $(SDK_DIR)/bin/* *.lua
	siftulator -e tests.lua
	touch $@

clean:
	rm -f tests.stamp

.PHONY: run clean

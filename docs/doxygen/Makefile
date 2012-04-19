DOXYGEN := doxygen
SDK_DIR := ../../sdk
OUTPUT_DIR := $(SDK_DIR)/doc
LOGFILE := doxygen.log

DEPS := \
	$(SDK_DIR)/include/sifteo.h \
	$(SDK_DIR)/include/sifteo/*.h \
	$(SDK_DIR)/include/sifteo/video/*.h \
	$(SDK_DIR)/include/sifteo/menu/*.h

# Run doxygen, and fail the build on any warnings.
all: $(LOGFILE)

$(LOGFILE): $(DEPS)
	rm -Rf $(OUTPUT_DIR)/*
	$(DOXYGEN)
	@ cat $(LOGFILE)
	@ ! grep -q warning $(LOGFILE)

clean:
	rm -Rf $(OUTPUT_DIR)/* $(LOGFILE)

.PHONY: all clean

DOXYGEN := doxygen
SDK_DIR := ../../sdk
OUTPUT_DIR := $(SDK_DIR)/doc
PROJECT_NUMBER = $(shell git describe --abbrev=0 --tags)
LOGFILE := doxygen.log

DEPS := \
	$(SDK_DIR)/include/sifteo.h \
	$(SDK_DIR)/include/sifteo/*.h \
	$(SDK_DIR)/include/sifteo/video/*.h \
	$(SDK_DIR)/include/sifteo/menu/*.h \
	resources/*.css \
	resources/*.html \
	resources/*.png \
	images/*.png \
	src/*.md

# Run doxygen, and fail the build on any warnings.
all: $(LOGFILE)

$(LOGFILE): $(DEPS)
	rm -Rf $(OUTPUT_DIR)/*
	DOXYGEN_OUTPUT_DIR=$(OUTPUT_DIR) DOXYGEN_PROJECT_NUMBER=$(PROJECT_NUMBER) $(DOXYGEN)
	@ cat $(LOGFILE)
	@ ! grep -q warning $(LOGFILE)

clean:
	rm -Rf $(OUTPUT_DIR)/* $(LOGFILE)

.PHONY: all clean

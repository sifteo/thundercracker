DOXYGEN := doxygen
OUTPUT_DIR := ../../sdk/doc
LOGFILE := doxygen.log

# Run doxygen, and fail the build on any warnings.
all:
	rm -Rf $(OUTPUT_DIR)/*
	$(DOXYGEN)
	@ cat $(LOGFILE)
	@ ! grep -q warning $(LOGFILE)

clean:
	rm -Rf $(OUTPUT_DIR)/* $(LOGFILE)

.PHONY: all clean


TESTS :=        \
	aes128
#   rfspectrum

# TODO: rfspectrum pulls in a lot of dependencies (most of siftulator), so i'm disabling
# it until we can do a better re-org of the way these get specified

    
all: $(TESTS)

$(TESTS):
	@$(MAKE) -C $@

clean:
	@for dir in $(TESTS); do $(MAKE) -C $$dir clean; done
	
.PHONY: clean $(TESTS)

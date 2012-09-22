
TESTS :=        \
	aes128      \
    rfspectrum
    
all: $(TESTS)

$(TESTS):
	@$(MAKE) -C $@

clean:
	@for dir in $(TESTS); do $(MAKE) -C $$dir clean; done
	
.PHONY: clean $(TESTS)

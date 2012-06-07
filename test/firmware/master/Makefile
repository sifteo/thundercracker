
TESTS := \
	aes128
    
all: $(TESTS)

$(TESTS):
	@$(MAKE) -C $@

clean:
	@for dir in $(TESTS); do $(MAKE) -C $$dir clean; done
	
.PHONY: clean $(TESTS)

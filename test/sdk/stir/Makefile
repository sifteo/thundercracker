OUTPUTS = \
	empty.html empty.gen.cpp empty.gen.h \
	empty-group.html empty-group.gen.cpp empty-group.gen.h

all: $(OUTPUTS)

clean:
	rm -f $(OUTPUTS)

.PHONY: all clean

%.html: %.lua
	stir -o $@ $< -v

%.gen.cpp: %.lua
	stir -o $@ $< -v

%.gen.h: %.lua
	stir -o $@ $< -v

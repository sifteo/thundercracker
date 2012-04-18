SUBDIRS = \
	chroma \
	drawtest \
	hello \
	membrane \
	menudemo \
	monster \
	nyan \
	pantera \
	sandwich \
	sensors \
	sprites \
	stars \
	text

.PHONY: clean subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@

clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done

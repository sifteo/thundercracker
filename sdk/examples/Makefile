SUBDIRS = \
	accelchart \
	assetslot \
	mandelbrot \
	membrane \
	menudemo \
	pairing \
	sensors \
	stampy \
	stars \
	synth \
	text

.PHONY: clean subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@

clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done

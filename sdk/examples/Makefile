SUBDIRS = \
	accelchart \
	assetslot \
	connection \
	mandelbrot \
	membrane \
	menudemo \
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

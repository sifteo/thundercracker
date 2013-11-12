SUBDIRS = \
	accelchart \
	assetslot \
	bluetooth \
	connection \
	mandelbrot \
	membrane \
	menudemo \
	sensors \
	stampy \
	stars \
	synth \
	text \
	usb

.PHONY: clean subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@

clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done

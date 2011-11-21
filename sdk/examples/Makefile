SUBDIRS = \
<<<<<<< HEAD
	hello monster text gfxdemo chroma sensors megademo \
	scroller-bg0 scroller-bg1 sprites stars tiltShake \
	pairing
=======
	hello monster text gfxdemo chroma sensors \
	scroller-bg0 scroller-bg1 sprites stars tiltShake
>>>>>>> master

.PHONY: clean subdirs $(SUBDIRS)

subdirs: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@

clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done

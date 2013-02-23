APP = assetslot

include $(SDK_DIR)/Makefile.defs

OBJS = $(ASSETS).gen.o main.o loader.o
ASSETDEPS += images/*.png images/spinny/*.png $(ASSETS).lua

# build assets.html to proof stir-processed assets.
# comment out to disable.
ASSETS_BUILD_PROOF := yes

include $(SDK_DIR)/Makefile.rules

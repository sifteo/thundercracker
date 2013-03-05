APP = connection

include $(SDK_DIR)/Makefile.defs

OBJS = $(ASSETS).gen.o main.o
ASSETDEPS += assets/*.png $(ASSETS).lua

# build assets.html to proof stir-processed assets.
# comment out to disable.
ASSETS_BUILD_PROOF := yes

include $(SDK_DIR)/Makefile.rules

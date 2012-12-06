APP = kaos

include $(SDK_DIR)/Makefile.defs

OBJS = $(ASSETS).gen.o main.o
ASSETDEPS += assets.lua kaos.png
GENERATED_FILES = kaos.png

include $(SDK_DIR)/Makefile.rules

kaos.png: kaos.py
	python kaos.py kaos.png
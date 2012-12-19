APP = slideshow

include $(SDK_DIR)/Makefile.defs

DATA_TOOLS = \
	BuildLua.py

GENERATED_FILES += assets.lua

OBJS = assets.gen.o main.o

ASSETDEPS += assets/*/*.png assets.lua

include $(SDK_DIR)/Makefile.rules

assets.lua: $(DATA_TOOLS) BuildLua.py assets/*/*.png 
	python BuildLua.py
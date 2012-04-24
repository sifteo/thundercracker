TC_DIR := ..

BIN_DIR := $(TC_DIR)/sdk/bin
BIN := siftulator

include Makefile.objs
include $(TC_DIR)/Makefile.platform
include $(TC_DIR)/firmware/master/Makefile.objs

# For faster 'make' invocation, track dependencies only for our own sources.
OBJS := $(OBJS_COMMON) $(LOCAL_OBJS)
DEPFILES := $(OBJS:.o=.d)

# All external libraries and library source files
include Makefile.libs

include Makefile.defs

CFLAGS := $(FLAGS) $(WARNFLAGS) $(INCLUDES)
CCFLAGS := $(FLAGS) $(WARNFLAGS) $(INCLUDES)
MFLAGS := $(FLAGS) $(INCLUDES)
LDFLAGS := $(FLAGS) $(LIBS)

include Makefile.rules

-include $(DEPFILES)

TC_DIR := ..
include $(TC_DIR)/Makefile.platform

BIN = slinky$(BIN_EXT)
all: $(BIN)

include Makefile.objs
DEPFILES := $(OBJS:.o=.d)

INCLUDES += -Isrc
FLAGS += -g
WARNFLAGS := -Werror -Wall -Wno-uninitialized

# LLVM
FLAGS += -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -fno-rtti
INCLUDES += -I$(LLVM_INC)
LIBS += \
	$(LLVM_LIB)/libLLVMInstCombine.a \
	$(LLVM_LIB)/libLLVMipo.a \
	$(LLVM_LIB)/libLLVMipa.a \
	$(LLVM_LIB)/libLLVMLinker.a \
	$(LLVM_LIB)/libLLVMScalarOpts.a \
	$(LLVM_LIB)/libLLVMTransformUtils.a \
	$(LLVM_LIB)/libLLVMSelectionDAG.a \
	$(LLVM_LIB)/libLLVMAnalysis.a \
	$(LLVM_LIB)/libLLVMBitReader.a \
	$(LLVM_LIB)/libLLVMInterpreter.a \
	$(LLVM_LIB)/libLLVMExecutionEngine.a \
	$(LLVM_LIB)/libLLVMAsmPrinter.a \
	$(LLVM_LIB)/libLLVMCodeGen.a \
	$(LLVM_LIB)/libLLVMTarget.a \
	$(LLVM_LIB)/libLLVMAsmParser.a \
	$(LLVM_LIB)/libLLVMMCDisassembler.a \
	$(LLVM_LIB)/libLLVMMCParser.a \
	$(LLVM_LIB)/libLLVMMC.a \
	$(LLVM_LIB)/libLLVMCore.a \
	$(LLVM_LIB)/libLLVMSupport.a
    
# Platform LLVM deps
ifeq ($(BUILD_PLATFORM), windows32)
    LIBS += -limagehlp -lpsapi
endif
    
# The DEBUG-ness of this build must match what LLVM was compiled with.
# Our libraries in deps are release builds, but if you want to set LLVM_LIB
# yourself, you can turn debugging back on.
ifeq ($(DEBUG),)
	FLAGS += -DNDEBUG
	FLAGS += -O3 -fomit-frame-pointer
endif

# Automatic dependency generation (*.d files)
FLAGS += -MMD

CCFLAGS := $(FLAGS) $(WARNFLAGS) $(INCLUDES)
LDFLAGS := $(FLAGS) $(LIBS) $(LIB_STDCPP)

$(BIN): $(OBJS) $(TABLES)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.cpp $(TABLES)
	$(CC) -c $(CCFLAGS) $*.cpp -o $*.o

.PHONY: clean

clean:
	rm -Rf $(BIN) $(OBJS) $(DEPFILES) $(TABLES) $(BIN).exe

-include $(DEPFILES)

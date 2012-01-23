TC_DIR := ..
include $(TC_DIR)/Makefile.platform

BIN = svmc

OBJS = src/llc.o

DEPFILES := $(OBJS:.o=.d)

FLAGS += -O3 -g -fomit-frame-pointer
WARNFLAGS := -Werror -Wall -Wno-uninitialized

# LLVM
FLAGS += -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS
INCLUDES += -I$(LLVM_INC)
LIBS += \
	$(LLVM_LIB)/libLLVMScalarOpts.a \
	$(LLVM_LIB)/libLLVMTransformUtils.a \
	$(LLVM_LIB)/libLLVMSelectionDAG.a \
	$(LLVM_LIB)/libLLVMAnalysis.a \
	$(LLVM_LIB)/libLLVMBitReader.a \
	$(LLVM_LIB)/libLLVMInterpreter.a \
	$(LLVM_LIB)/libLLVMExecutionEngine.a \
	$(LLVM_LIB)/libLLVMCodeGen.a \
	$(LLVM_LIB)/libLLVMTarget.a \
	$(LLVM_LIB)/libLLVMAsmParser.a \
	$(LLVM_LIB)/libLLVMAsmPrinter.a \
	$(LLVM_LIB)/libLLVMMCParser.a \
	$(LLVM_LIB)/libLLVMMC.a \
	$(LLVM_LIB)/libLLVMCore.a \
	$(LLVM_LIB)/libLLVMSupport.a

# Automatic dependency generation (*.d files)
FLAGS += -MMD

CCFLAGS := $(FLAGS) $(WARNFLAGS) $(INCLUDES)
LDFLAGS := $(LIB_STDCPP) $(FLAGS) $(LIBS)

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CC) -c $(CCFLAGS) $*.cpp -o $*.o

.PHONY: clean

clean:
	rm -Rf $(BIN) $(OBJS) $(DEPFILES) $(BIN).exe

-include $(DEPFILES)

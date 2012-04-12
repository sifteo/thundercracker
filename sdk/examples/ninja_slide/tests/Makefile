
TC_DIR = ../../../..
SRC_DIR = ..
TEST_DIR = .
GTEST_DIR = $(TC_DIR)/deps/src/gtest-1.6.0
SIFTEO_SDK_DIR = $(TC_DIR)/sdk/include

TEST_SRCS = $(wildcard $(TEST_DIR)/*_tests.cpp)
TEST_OBJS = $(patsubst %.cpp,%.o,$(TEST_SRCS))


# Flags passed to the preprocessor.
CPPFLAGS += -I$(SRC_DIR) -I$(GTEST_DIR)/include -I$(SIFTEO_SDK_DIR)  

# Flags passed to the C++ compiler.
CXXFLAGS += -g -Wall -Wextra

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

# House-keeping build targets.

all : alltests gtest_main.a 
	./alltests

clean :
	rm -f $(TESTS) gtest.a gtest_main.a *.o

alltests: $(TEST_OBJS) gtest_main.a $(IMPL_SRCS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@

%.o: %.cpp $(patsubst %_test.cpp,%.h,$(SRC_DIR)) $(CDEPS)
	echo %.cpp $(SRC_DIR)/%.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $^


# Builds gtest.a and gtest_main.a.

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

gtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^





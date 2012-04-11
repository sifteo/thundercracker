# All tests produced by this Makefile.  Remember to add new tests you
# created to the list.
TESTS = rect_tests

TC_DIR = ../../../..
USER_DIR = ..
TEST_DIR = .
GTEST_DIR = $(TC_DIR)/deps/src/gtest-1.6.0
SIFTEO_SDK_DIR = $(TC_DIR)/sdk/include

# Flags passed to the preprocessor.
CPPFLAGS += -I$(GTEST_DIR)/include -I$(SIFTEO_SDK_DIR) -I$(USER_DIR)

# Flags passed to the C++ compiler.
CXXFLAGS += -g -Wall -Wextra

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

# House-keeping build targets.

all : $(TESTS) 
	./rect_tests

clean :
	rm -f $(TESTS) gtest.a gtest_main.a *.o

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

# Builds a sample test.  A test should link with either gtest.a or
# gtest_main.a, depending on whether it defines its own main()
# function.

# sample1.o : $(USER_DIR)/sample1.cc $(USER_DIR)/sample1.h $(GTEST_HEADERS)
#         $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(USER_DIR)/sample1.cc

# test.o : $(TEST_DIR)/test.cpp
#         $(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(TEST_DIR)/test.cpp

rect_tests: rect_tests.o gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@

%.o: %.cpp $(CDEPS)
	$(CXX) $(CPPFLAGS) -c -o $@ $< $(CXXFLAGS)


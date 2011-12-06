TESTS = testcube

all : $(TESTS)

clean :
	rm -f $(TESTS) gtest.a gtest_main.a *.o


include ../Makefile.test
include ../Makefile.defs
include ../Makefile.rules

CPPFLAGS += -DBUILD_UNIT_TEST -DUSE_MOCK_CUBE_CODEC

main.o : main.cpp $(GTEST_HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c main.cpp

cube_test.o : cube_test.cpp $(GTEST_HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c cube_test.cpp

testcube : cube_test.o main.o gtest_main.a $(BIN_SIM)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@

run :
	./testcube

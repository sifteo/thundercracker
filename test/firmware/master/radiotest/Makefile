TESTS = testradio

all : $(TESTS)

clean :
	rm -f $(TESTS) gtest.a gtest_main.a *.o


include ../Makefile.test
include ../Makefile.defs
include ../Makefile.rules

CPPFLAGS += -DBUILD_UNIT_TEST -DUSE_MOCK_CUBE

main.o : main.cpp $(GTEST_HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c main.cpp

radio_test.o : radio_test.cpp $(GTEST_HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c radio_test.cpp

testradio : radio_test.o main.o gtest_main.a $(BIN_SIM)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@

run :
	./testradio

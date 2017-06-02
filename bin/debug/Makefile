CXXFLAGS = -Wall -W -g -std=c++14 -fstack-protector -Wshadow -Wformat-security -fconcepts -fsanitize=address -fsanitize-recover=address -fsanitize=undefined -fsanitize=vptr -msse4.1
LDFLAGS = 
CXX := g++

correctness_tests: ../../src/correctness_tests.cpp
	$(CXX) $(CXXFLAGS) ../../src/correctness_tests.cpp -o correctness_tests $(LDFLAGS)

speed_tests: ../../src/speed_tests.cpp 	
	$(CXX) $(CXXFLAGS) ../../src/speed_tests.cpp -o speed_tests $(LDFLAGS)

sstring: ../../src/sstring.cpp
	$(CXX) $(CXXFLAGS) ../../src/sstring.cpp -o sstring $(LDFLAGS)

all: correctness_tests speed_tests sstring

clean:
	rm -rf correctness_tests speed_tests sstring

distclean: clean


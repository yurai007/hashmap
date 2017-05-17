CXXFLAGS = -Wall -W -g -Ofast -std=c++14 -Wshadow -Wformat-security -fconcepts 
LDFLAGS = 
CXX := g++

all: ../../src/main.cpp 
	$(CXX) $(CXXFLAGS) ../../src/main.cpp -o main $(LDFLAGS)

clean:
	rm -rf main

distclean: clean


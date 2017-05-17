CXXFLAGS = -Wall -W -g -std=c++14 -fstack-protector -Wshadow -Wformat-security -fconcepts -fsanitize=address -fsanitize-recover=address -fsanitize=undefined -fsanitize=vptr
LDFLAGS = 
CXX := g++

all: ../../src/main.cpp 
	$(CXX) $(CXXFLAGS) ../../src/main.cpp -o main $(LDFLAGS)

clean:
	rm -rf main

distclean: clean


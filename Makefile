# Temporary make buildsystem until
# I do something not make
CXX = clang++
CXXFLAGS = -std=c++23 -O3

all: test

test: test.o
	$(CXX) $(CXXFLAGS) $^ -lcrypto -lz -o $@

%.o: src/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@


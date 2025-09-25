CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20
SRC = $(wildcard *.cpp ./lexer/*.cpp ./parser/*.cpp ./ast/*.cpp)
OBJ = $(SRC:.cpp=.o)
TARGET = main

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20
SRC = $(wildcard *.cpp ./lexer/*.cpp ./parser/*.cpp ./ast/*.cpp)
OBJ = $(SRC:.cpp=.o)
TARGET = main.exe

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	ifeq ($(OS),Windows_NT)
		powershell -Command "Remove-Item -Force -ErrorAction SilentlyContinue $(OBJ) $(TARGET)"
	else
		rm -f $(OBJ) $(TARGET)
	endif

debug:
	$(MAKE) CXXFLAGS="-Wall -Wextra -std=c++20 -g -DDEBUG -O0"


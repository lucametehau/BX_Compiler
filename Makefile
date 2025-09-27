CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++20
TARGET = main.exe

ifeq ($(OS),Windows_NT)
	SRC_TEMP = $(wildcard *.cpp) $(wildcard lexer/*.cpp) $(wildcard parser/*.cpp) $(wildcard ast/*.cpp)
	SRC = $(subst /,\,$(SRC_TEMP))
    OBJ = $(SRC:.cpp=.o)
    DEPS = $(OBJ:.o=.d)
	
	comma := ,
	space := $(subst ,, )
	FILES_TEMP := $(OBJ) $(TARGET) $(DEPS)
	FILES := $(subst $(space),$(comma) ,$(FILES_TEMP))
	CLEAN_CMD := powershell "rm -Force $(FILES)"
else
    SRC = $(wildcard *.cpp) $(wildcard ./lexer/*.cpp) $(wildcard ./parser/*.cpp) $(wildcard ./ast/*.cpp)
    OBJ = $(SRC:.cpp=.o)
    DEPS = $(OBJ:.o=.d)
	CLEAN_CMD := rm -f $(OBJ) $(TARGET) $(DEPS)
endif

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^


%.o: %.cpp
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

clean:
	$(CLEAN_CMD)

debug:
	$(MAKE) CXXFLAGS="-Wall -Wextra -std=c++20 -g -DDEBUG -O0"

-include $(DEPS)


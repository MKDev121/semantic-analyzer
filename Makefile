CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -Isrc

SRC = src/Token.hpp \
      src/Lexer.cpp \
      src/Type.cpp \
      src/SymbolTable.cpp \
      src/AST.cpp \
      src/Parser.cpp \
      src/SemanticAnalyzer.cpp \
      src/main.cpp

OBJ = $(patsubst src/%.cpp, build/%.o, $(filter %.cpp, $(SRC)))

TARGET = semantic_analyzer

all: $(TARGET)

build:
	mkdir -p build

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -rf build $(TARGET)

test: $(TARGET)
	chmod +x run_tests.sh
	./run_tests.sh

.PHONY: all clean test

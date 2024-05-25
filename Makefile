EXENAME = 6502
OBJ_DIR = obj/
CXX = clang++ -std=c++20 -Wall -Wextra -Werror
SOURCES = main.cpp cpu.cpp
OBJECTS = $(SOURCES:%.cpp=$(OBJ_DIR)%.o)
VPATH = src:

all: $(EXENAME)

$(EXENAME): $(OBJECTS)
	$(CXX) $^ -o $@

$(OBJ_DIR)main.o: main.cpp cpu.hpp
	$(CXX) -c $< -o $@

$(OBJ_DIR)cpu.o: cpu.cpp cpu.hpp
	$(CXX) -c $< -o $@

.PHONY: clean run

clean:
	rm -f $(EXENAME) $(OBJ_DIR)*.o
run:
	./$(EXENAME)
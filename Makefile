EXENAME = 6502
OBJ_DIR = obj/
CXX = clang++ -std=c++20 -g -Wall -Wextra -Werror 
SOURCES = main.cpp emulator.cpp cpu.cpp memory.cpp
OBJECTS = $(SOURCES:%.cpp=$(OBJ_DIR)%.o)
VPATH = src:

all: $(EXENAME)

$(EXENAME): $(OBJECTS)
	$(CXX) $^ -o $@ -lncursesw -lpthread

$(OBJ_DIR)main.o: main.cpp emulator.hpp
	$(CXX) -c $< -o $@

$(OBJ_DIR)emulator.o: emulator.cpp emulator.hpp cpu.hpp memory.hpp common.hpp
	$(CXX) -c $< -o $@

$(OBJ_DIR)cpu.o: cpu.cpp cpu.hpp common.hpp
	$(CXX) -c $< -o $@

$(OBJ_DIR)memory.o: memory.cpp memory.hpp common.hpp
	$(CXX) -c $< -o $@



.PHONY: clean run debug

clean:
	rm -f $(EXENAME) $(OBJ_DIR)*.o
run:
	./$(EXENAME)

debug:
	rm -f out.txt
	./${EXENAME} >> out.txt


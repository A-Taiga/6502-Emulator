# EXENAME = 6502
# CXX = clang++
# CXXFLAGS = -std=c++20 -g -Wall -Wextra -Werror -Wformat -I imgui -I include
# UNAME_S := $(shell uname -s)
# LINUX_GL_LIBS = -lGL
# VPATH = src: imgui: include:
# OBJ_DIR = obj/
# SOURCES =  main.cpp cpu.cpp memory.cpp emulator.cpp bus.cpp window.cpp debug.cpp imgui.cpp imgui_demo.cpp imgui_draw.cpp imgui_tables.cpp
# SOURCES += imgui_widgets.cpp imgui_impl_sdl2.cpp imgui_impl_sdlrenderer2.cpp
# OBJECTS = $(SOURCES:%.cpp=$(OBJ_DIR)%.o)
# LIBS = 

# ifeq ($(UNAME_S), Linux)
# 	ECHO_MESSAGE = "Linux compiled"
# 	LIBS += -lGL -ldl `sdl2-config --libs`
# 	CXXFLAGS += `sdl2-config --cflags`
# 	CFLAGS = $(CXXFLAGS)
# endif

# ifeq ($(UNAME_S), Darwin)
# 	ECHO_MESSAGE = "OS X compiled"
# 	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo `sdl2-config --libs`
# 	LIBS += -L/usr/local/lib
# 	CXXFLAGS += `sdl2-config --cflags`
# 	CXXFLAGS += -I/usr/local/include -I/opt/local/include
# 	CFLAGS = $(CXXFLAGS)
# endif

# all: $(EXENAME)
# 	@echo $(ECHO_MESSAGE)

# $(EXENAME): $(OBJECTS)
# 	$(CXX) -o $@ $^ $(CXXFLAG) $(LIBS) -lncurses

# $(OBJECTS): $(OBJ_DIR)%.o: %.cpp
# 	$(CXX) $(CXXFLAGS) -c -o $@ $<

# .PHONY: clean run

# run:
# 	./$(EXENAME)

# clean:
# 	rm -f $(EXENAME) $(OBJECTS)
EXE = 6502
VPATH = src:
IMGUI_DIR = imgui
OBJ_DIR = .obj
SOURCES = main.cpp cpu.cpp memory.cpp emulator.cpp bus.cpp window.cpp
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_sdl2.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
OBJS = $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(basename $(notdir $(SOURCES)))))
UNAME_S := $(shell uname -s)
LINUX_GL_LIBS = -lGL
CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -g -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -Iinclude
LIBS = 

ifeq ($(UNAME_S), Linux)
	ECHO_MESSAGE = "Linux"
	LIBS += $(LINUX_GL_LIBS) -ldl `sdl2-config --libs`

	CXXFLAGS += `sdl2-config --cflags`
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) 
	ECHO_MESSAGE = "Mac OS X"
	LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo `sdl2-config --libs`
	LIBS += -L/usr/local/lib -L/opt/local/lib

	CXXFLAGS += `sdl2-config --cflags`
	CXXFLAGS += -I/usr/local/include -I/opt/local/include
	CFLAGS = $(CXXFLAGS)
endif

.PHONY: clean run

$(OBJ_DIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: $(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)
run:
	./$(EXE)

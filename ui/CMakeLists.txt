find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

add_library(GUI
    ../imgui/imgui.cpp
    ../imgui/imgui_demo.cpp
    ../imgui/imgui_draw.cpp
    ../imgui/imgui_tables.cpp
    ../imgui/imgui_widgets.cpp
    ../imgui/backends/imgui_impl_sdl2.cpp
    ../imgui/backends/imgui_impl_opengl3.cpp
    src/window.cpp
    src/debugger.cpp
    src/hex_editor.cpp
)

target_include_directories(GUI PUBLIC 
    ${PROJECT_SOURCE_DIR}/ui/include
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${PROJECT_SOURCE_DIR}/imgui
    ${PROJECT_SOURCE_DIR}/imgui/backends
    ${PROJECT_SOURCE_DIR}/ui
)

target_link_libraries(GUI PUBLIC SDL2main SDL2)
target_link_libraries(GUI PUBLIC GL)
target_link_libraries(GUI PUBLIC CPU)

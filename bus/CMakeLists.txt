add_library (BUS "src/bus.cpp")
target_include_directories(BUS PUBLIC ${PROJECT_SOURCE_DIR}/bus/include)
target_link_libraries(BUS PUBLIC MEMORY)
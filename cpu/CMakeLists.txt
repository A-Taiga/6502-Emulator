add_library (CPU "src/mos6502.cpp")
target_include_directories(CPU PUBLIC ${PROJECT_SOURCE_DIR}/cpu/include)
target_link_libraries(CPU PUBLIC MEMORY)
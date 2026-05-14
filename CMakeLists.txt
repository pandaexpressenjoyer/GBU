cmake_minimum_required(VERSION 3.15)
project(GameBoyEmulator VERSION 1.0)

# Specify C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Gather all your source files
file(GLOB SOURCES "src/*.cpp")

# Create the executable
add_executable(gbu_emu ${SOURCES})

# Tell CMake where your custom header folders are
target_include_directories(gbu_emu PRIVATE
    "${CMAKE_SOURCE_DIR}/inc"
    "${CMAKE_SOURCE_DIR}/dependencies/include"
)

# Tell CMake where your custom library files are
target_link_directories(gbu_emu PRIVATE
    "${CMAKE_SOURCE_DIR}/dependencies/lib"
)

# Link the specific SDL2 libraries
# MinGW requires 'mingw32' to be linked before SDL2main on Windows
target_link_libraries(gbu_emu PRIVATE mingw32 SDL2main SDL2)

# Automatically copy SDL2.dll to the build directory after compiling
add_custom_command(TARGET gbu_emu POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/SDL2.dll"
    $<TARGET_FILE_DIR:gbu_emu>
)
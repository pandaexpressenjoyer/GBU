# 1. Compiler and Flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -g

# 2. Project Files
SRCS = src/main.cpp src/CPU.cpp src/PPU.cpp src/Memory.cpp src/Controller.cpp
TARGET = gbu_emu.exe

# 3. Include and Library Paths (Mapping to your specific folders)
INCLUDES = -I./inc -I./dependencies/include
LIB_DIRS = -L./dependencies/lib

# 4. Libraries to Link
# CRITICAL: MinGW requires mingw32 to be linked BEFORE SDL2main and SDL2!
LIBS = -lmingw32 -lSDL2main -lSDL2

# 5. Build Rules
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) $(LIB_DIRS) $(LIBS) -o $(TARGET)

# 6. Cleanup (Uses 'del' for Windows Command Prompt)
clean:
	del $(TARGET)
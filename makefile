# 1. Compiler and Flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -g

# 2. Project Files (Added Timer.cpp here!)
SRCS = src/main.cpp src/CPU.cpp src/PPU.cpp src/Memory.cpp src/Controller.cpp src/Cartridge.cpp src/Timer.cpp
TARGET = gbu_emu.exe

# 3. Include and Library Paths 
INCLUDES = -I./inc -I./dependencies/include
LIB_DIRS = -L./dependencies/lib

# 4. Libraries to Link
LIBS = -lmingw32 -lSDL2main -lSDL2

# 5. Build Rules
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRCS) $(LIB_DIRS) $(LIBS) -o $(TARGET)

# 6. Cleanup
clean:
	del $(TARGET)
# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2

# Files
SRCS = BufferData.cpp LEFMData.cpp main.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = lefm_app

# Default target
all: $(TARGET)

# Link objects
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Compile source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean


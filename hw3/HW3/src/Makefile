CXX = g++
CXXFLAGS = -std=c++17 -O2
TARGET = hw3
SRCS = main.cpp Floorplanner.cpp Timer.cpp
OBJS = $(SRCS:.cpp=.o)
BIN_DIR = ../bin

all: $(BIN_DIR)/$(TARGET)

$(BIN_DIR)/$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f *.o $(BIN_DIR)/$(TARGET)

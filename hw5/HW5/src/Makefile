# --- 編譯器 ---
CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -I.

# --- 來源檔 (.cpp 全在目前資料夾) ---
SRCS     := $(wildcard *.cpp)
TARGET   := ../bin/hw5

release:
	@mkdir -p ../bin
	$(CXX) $(SRCS) -o $(TARGET) $(CXXFLAGS)

clean:
	@rm -f $(TARGET)
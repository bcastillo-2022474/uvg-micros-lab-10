CXX      := g++
CXXFLAGS := -Wall -Wextra -pthread -std=c++17 -I src/include
LDFLAGS  := -lz -lpthread

SRC_DIR   := src
BUILD_DIR := build

TARGET := $(BUILD_DIR)/parallel_compression

SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

.PHONY: all clean run deps-ubuntu deps-fedora

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

# Install zlib development headers
deps-ubuntu:
	sudo apt install -y zlib1g-dev

deps-fedora:
	sudo dnf install -y zlib-devel

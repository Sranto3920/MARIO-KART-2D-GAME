CXX = clang++

SFML_PATH = /opt/homebrew/Cellar/sfml@2/2.6.2_1
cppFileNames := $(shell find ./src -type f -name "*.cpp")

all: compile

compile:
	mkdir -p bin
	$(CXX) -std=c++17 -arch arm64 $(cppFileNames) -I$(SFML_PATH)/include -o bin/main -L$(SFML_PATH)/lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lsfml-network

clean:
	rm -rf bin

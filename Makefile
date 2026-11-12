# Host-side unit tests for the portable motion/command core (no ESP32 needed).
CXX ?= g++
build/test_core: test/host/test_core.cpp lib/rotator/rotator_core.cpp lib/rotator/rotator_core.h
	mkdir -p build
	$(CXX) -std=c++17 -Wall -Wextra -Ilib/rotator -o $@ test/host/test_core.cpp lib/rotator/rotator_core.cpp

test: build/test_core
	./build/test_core

.PHONY: test

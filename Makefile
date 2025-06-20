################################################################################
# Makefile for TCLI Project
#
# This Makefile orchestrates the build process for the TCLI C++20 project,
# leveraging Clang's experimental C++ modules support. It is designed to:
#   - Build module interface units (.cppm) and generate corresponding PCM files.
#   - Compile implementation files (.cpp), properly importing modules where needed.
#   - Link all object files into the final executable.
#   - Manage build and module output directories.
#   - Provide a clean target to remove all build artifacts.
#
# Key Variables:
#   CXX        : C++ compiler (clang++)
#   CXXFLAGS   : Compiler flags for C++20, modules, libc++, and threading
#   LDFLAGS    : Linker flags for libc++ and threading
#   TARGET     : Name of the final executable
#   SRC_DIR    : Directory containing source files
#   BUILD_DIR  : Directory for object files
#   MOD_DIR    : Directory for module PCM files
#   MODULES    : List of module interface source files (.cppm)
#   SRCS       : List of implementation source files (.cpp)
#   MOD_OBJS   : Object files generated from module interfaces
#   OBJS       : Object files generated from implementation sources
#   PCMS       : PCM files generated from module interfaces
#
# Build Targets:
#   all        : Default target; builds the final executable.
#   $(TARGET)  : Links all object files into the executable.
#   $(BUILD_DIR)/%.o : Compiles module interface units and generates PCM files.
#   $(BUILD_DIR)/main.o : Compiles main.cpp, importing the common module.
#   $(BUILD_DIR)/platform.o : Compiles platform.cpp without module imports.
#   $(MOD_DIR) : Ensures the module directory exists.
#   $(BUILD_DIR) : Ensures the build directory exists.
#   clean      : Removes all build artifacts and the executable.
#
# Notes:
#   - Only main.cpp is compiled with module imports, as it uses the common module.
#   - platform.cpp is compiled without module imports to avoid issues with system headers.
#   - Directories are created as needed to ensure build consistency.
#   - This Makefile assumes Clang++ with experimental C++20 modules support.
################################################################################
CXX := clang++
CXXFLAGS := -std=c++20 -fmodules -fcxx-modules -stdlib=libc++ -pthread
LDFLAGS := -lc++ -lc++abi -pthread

TARGET := tcli

SRC_DIR := src
BUILD_DIR := build
MOD_DIR := $(BUILD_DIR)/modules

# Source files
MODULES := common.cppm
SRCS := main.cpp platform.cpp

# Objects
MOD_OBJS := $(patsubst %.cppm,$(BUILD_DIR)/%.o,$(MODULES))
OBJS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS))

# PCM files
PCMS := $(patsubst %.cppm,$(MOD_DIR)/%.pcm,$(MODULES))

.PHONY: all clean

all: $(TARGET)

# Link all objects to create target
$(TARGET): $(MOD_OBJS) $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compile module interface and generate PCM
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cppm | $(MOD_DIR) $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -fmodule-output=$(MOD_DIR)/$*.pcm -c $< -o $@

# Compile cpp files importing modules, pass -fmodule-file for common.pcm
# But **only** for files that import modules (i.e., main.cpp)
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.cpp $(PCMS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -fmodule-file=common=$(MOD_DIR)/common.pcm -c $< -o $@

# Compile platform.cpp WITHOUT modules because it includes system headers
$(BUILD_DIR)/platform.o: $(SRC_DIR)/platform.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create directories if needed
$(MOD_DIR):
	mkdir -p $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

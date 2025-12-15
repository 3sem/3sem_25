# Makefile for managing CMake assemblies

# Loading .env file if it exist
ifneq (,$(wildcard ./.env))
	include .env
	export
endif

# Project configurations
PROJECT_NAME := $(PROJECT_NAME)
BUILD_DIR := build
SOURCE_DIR := src
DEBUG_BUILD_DIR := $(BUILD_DIR)/Debug
RELEASE_BUILD_DIR := $(BUILD_DIR)/Release
INSTALL_DIR := /usr/local/bin
PROJECT_ROOT := $(shell pwd)

# Cmake flags
CMAKE := cmake
CMAKE_FLAGS := -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# Default targets
.DEFAULT_GOAL := default

# Default target
.PHONY: default
default: env help

# Show env variables from .env file
.PHONY: env
env:
	@echo "=== Environment Variables ==="
	@if [ -f .env ]; then \
		echo "Loading .env file..."; \
		echo "Available variables:"; \
		grep -v '^#' .env | grep -v '^$$' | while read line; do \
			var=$$(echo "$$line" | cut -d'=' -f1); \
			value=$$(echo "$$line" | cut -d'=' -f2-); \
			echo "  $$var=$$value"; \
		done; \
	else \
		echo "No .env file found"; \
	fi

# Show help
.PHONY: help
help:
	@echo "=== Available targets ==="
	@echo "  debug     		- Build debug version"
	@echo "  release   		- Build release version"
	@echo "  all       		- Build both versions"
	@echo "  configure-debug 	- Config debug version"
	@echo "  configure-release 	- Config release version"
	@echo "  configure    		- Configure both options"
	@echo "  run-debug    		- Run debug version"
	@echo "  run-release  		- Run release version"
	@echo "  list-src		- Show source files"
	@echo "  build-src     	- Build single file (usage: make build-src FILE=filename)"
	@echo "  run-src     		- Build and run single file (usage: make run-src FILE=filename [ARGS=\"arguments\"])"
	@echo "  run-src-debug     	- Build and run single file in debug mode (usage: make run-src-debug FILE=filename [ARGS=\"arguments\"])"
	@echo "  run-src-release     	- Build and run single file in release mode (usage: make run-src-release FILE=filename [ARGS=\"arguments\"])"
	@echo "  clean-debug  		- Clear debug assemble"
	@echo "  clean-release 	- Clear release assemble"
	@echo "  clean     		- Clear all assemblies"
	@echo "  rebuild-debug  	- Rebuild debug version"
	@echo "  rebuild-release 	- Rebuild release version"
	@echo "  rebuild     		- Rebuild both versions"
	@echo "  env         		- Show environment variables"
	@echo "  help         		- Show help"

# Debug assemble
.PHONY: debug
debug: configure-debug
	@echo "Build debug version..."
	@$(CMAKE) --build $(DEBUG_BUILD_DIR) --config Debug

# Release assemble
.PHONY: release
release: configure-release
	@echo "Build release version..."
	@$(CMAKE) --build $(RELEASE_BUILD_DIR) --config Release

# Both assemblies
.PHONY: all
all: debug release

# Config debug version
.PHONY: configure-debug
configure-debug:
	@echo "Config debug version..."
	@mkdir -p $(DEBUG_BUILD_DIR)
	@cd $(DEBUG_BUILD_DIR) && $(CMAKE) $(PROJECT_ROOT) -DCMAKE_BUILD_TYPE=Debug $(CMAKE_FLAGS)

# Config release version
.PHONY: configure-release
configure-release:
	@echo "Config release version..."
	@mkdir -p $(RELEASE_BUILD_DIR)
	@cd $(RELEASE_BUILD_DIR) && $(CMAKE) $(PROJECT_ROOT) -DCMAKE_BUILD_TYPE=Release $(CMAKE_FLAGS)

# Config both versions
.PHONY: configure
configure: configure-debug configure-release

# Run debug version
.PHONY: run-debug
run-debug: debug
	@echo "Run debug version..."
	@./$(DEBUG_BUILD_DIR)/$(PROJECT_NAME) $(ARGS)

# Run release version
.PHONY: run-release
run-release: release
	@echo "Run release version..."
	@./$(RELEASE_BUILD_DIR)/$(PROJECT_NAME) $(ARGS)

# Show available source files in src
.PHONY: list-src
list-src:
	@echo "=== Available source files in $(SOURCE_DIR)/ ==="
	@if [ -d "$(SOURCE_DIR)" ]; then \
		for file in $(SOURCE_DIR)/*.cpp $(SOURCE_DIR)/*.c; do \
			if [ -f "$$file" ]; then \
				filename=$$(basename "$$file"); \
				echo "  $$filename"; \
			fi; \
		done; \
	else \
		echo "Source directory $(SOURCE_DIR) not found"; \
	fi

# Build single file using CMake
.PHONY: build-src
build-src: configure-debug
	@if [ -z "$(FILE)" ]; then \
		echo "Error: Please specify FILE parameter (filename without extension)"; \
		echo "Usage: make build-src FILE=filename"; \
		echo ""; \
		echo "Available source files:"; \
		$(MAKE) list-src; \
		exit 1; \
	fi
	@echo "Building $(FILE) with CMake..."
	@if cd $(DEBUG_BUILD_DIR) && $(CMAKE) --build . --target $(FILE); then \
		echo "✓ Successfully built $(FILE)"; \
	else \
		echo "✗ Failed to build $(FILE) with CMake"; \
		exit 1; \
	fi

# Build single file in release mode
.PHONY: build-src-release
build-src-release: configure-release
	@if [ -z "$(FILE)" ]; then \
		echo "Error: Please specify FILE parameter (filename without extension)"; \
		echo "Usage: make build-src-release FILE=filename"; \
		echo ""; \
		echo "Available source files:"; \
		$(MAKE) list-src; \
		exit 1; \
	fi
	@echo "Building $(FILE) in release mode with CMake..."
	@if cd $(RELEASE_BUILD_DIR) && $(CMAKE) --build . --target $(FILE); then \
		echo "✓ Successfully built $(FILE) in release mode"; \
	else \
		echo "✗ Failed to build $(FILE) with CMake"; \
		exit 1; \
	fi

# Run single file using CMake (debug mode by default)
.PHONY: run-src
run-src: build-src
	@echo "Running $(FILE) with arguments: $(ARGS)"
	@./$(DEBUG_BUILD_DIR)/$(FILE) $(ARGS)

# Run single file in debug mode with arguments
.PHONY: run-src-debug
run-src-debug: build-src
	@echo "Running $(FILE) in debug mode with arguments: $(ARGS)"
	@./$(DEBUG_BUILD_DIR)/$(FILE) $(ARGS)

# Run single file in release mode with arguments
.PHONY: run-src-release
run-src-release: build-src-release
	@echo "Running $(FILE) in release mode with arguments: $(ARGS)"
	@./$(RELEASE_BUILD_DIR)/$(FILE) $(ARGS)

# Clear debug assemble
.PHONY: clean-debug
clean-debug:
	@echo "Clear debug assemble..."
	@rm -rf $(DEBUG_BUILD_DIR)

# Clear release assemble
.PHONY: clean-release
clean-release:
	@echo "Clear release assemble..."
	@rm -rf $(RELEASE_BUILD_DIR)

# Complete clean
.PHONY: clean
clean: clean-debug clean-release
	@echo "Complete clean..."
	@rm -rf $(BUILD_DIR)

# Debug reassembly
.PHONY: rebuild-debug
rebuild-debug: clean-debug debug

# Release reassembly
.PHONY: rebuild-release
rebuild-release: clean-release release

# Complete reassembly
.PHONY: rebuild
rebuild: clean all

# Show information about assemblies
.PHONY: info
info:
	@echo "=== Project information ==="
	@echo "Project name: $(PROJECT_NAME)"
	@echo "Debug directory: $(DEBUG_BUILD_DIR)"
	@echo "Release directory: $(RELEASE_BUILD_DIR)"
	@echo "CMake flags: $(CMAKE_FLAGS)"
	@if [ -d "$(DEBUG_BUILD_DIR)" ]; then \
		echo "Debug assemble: exists"; \
	else \
		echo "Debug assemble: not configured"; \
	fi
	@if [ -d "$(RELEASE_BUILD_DIR)" ]; then \
		echo "Release assemble: exists"; \
	else \
		echo "Release assemble: not configured"; \
	fi

# Create symlinks
.PHONY: links
links:
	@echo "Creating symlinks..."
	@ln -sf $(DEBUG_BUILD_DIR)/$(PROJECT_NAME) ./debug-$(PROJECT_NAME) 2>/dev/null || true
	@ln -sf $(RELEASE_BUILD_DIR)/$(PROJECT_NAME) ./release-$(PROJECT_NAME) 2>/dev/null || true
	@echo "Created symlinks:"
	@echo "  debug-$(PROJECT_NAME) -> $(DEBUG_BUILD_DIR)/$(PROJECT_NAME)"
	@echo "  release-$(PROJECT_NAME) -> $(RELEASE_BUILD_DIR)/$(PROJECT_NAME)"

# Delete links
.PHONY: clean-links
clean-links:
	@echo "Cleaning symbolic links..."
	@-rm -f ./debug-$(PROJECT_NAME)* ./release-$(PROJECT_NAME)* ./$(PROJECT_NAME)* 2>/dev/null || true
	@echo "Symbolic links removed"

.DEFAULT_GOAL := all
TARGET = Basie
CPP_SOURCES = Basie.cpp $(wildcard src/*.cpp)
LIBDAISY_DIR ?= vendor/libDaisy
DAISYSP_DIR ?= vendor/DaisySP
GCC_PATH ?= $(if $(wildcard vendor/toolchain/bin/arm-none-eabi-g++),$(abspath vendor/toolchain/bin),)
USE_FATFS = 1
CPP_STANDARD = -std=gnu++17
OPT = -Os

.PHONY: test test-sanitize deps libs check-toolchain
test:
	python3 scripts/run_tests.py

test-sanitize:
	python3 scripts/run_tests.py --sanitize

deps:
	python3 scripts/bootstrap.py

# Host tests and SDK fetching work without the ARM compiler or installed SDKs.
HOST_GOALS := test test-sanitize deps
HOST_ONLY := $(if $(MAKECMDGOALS),$(if $(filter-out $(HOST_GOALS),$(MAKECMDGOALS)),,1),)
ifeq ($(HOST_ONLY),)
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
ifeq ($(wildcard $(SYSTEM_FILES_DIR)/Makefile),)
$(error Daisy SDK missing. Run 'make deps', then 'make libs' with Arm GNU 13.3.Rel1)
endif
include $(SYSTEM_FILES_DIR)/Makefile

check-toolchain:
	@$(CXX) --version | head -n 1
	@test "$$($(CXX) -dumpfullversion)" = "13.3.1" || \
	  (echo "Expected Arm GNU Toolchain 13.3.Rel1 (GCC 13.3.1). Set GCC_PATH to its bin directory."; exit 1)

libs: check-toolchain
	$(MAKE) -C $(LIBDAISY_DIR) GCC_PATH="$(GCC_PATH)"
	$(MAKE) -C $(DAISYSP_DIR) GCC_PATH="$(GCC_PATH)"

$(BUILD_DIR)/$(TARGET).elf: | check-toolchain
endif

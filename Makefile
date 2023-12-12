# Project Name
TARGET = Basie

# Source Directory
SRC_DIR = src

# Add Basie.cpp along with all .cpp files from the src directory
CPP_SOURCES = Basie.cpp $(wildcard $(SRC_DIR)/*.cpp)

# Library Locations
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP

USE_FATFS = 1

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

OPT = -Os


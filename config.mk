# LeoCAD configuration file
#
include version.mk

CC		:= gcc
CXX		:= g++

ifeq ($(shell uname), linux)
OS 		:= -DLC_LINUX
OSDIR 	:= linux

else

ifeq ($(shell uname), BeOS)
OS 		:= -DLC_BEOS
OSDIR 	:= beos
endif

endif

# (Add a -g for debugging)
CPPFLAGS += -O2 -Wall

# Add compile options, such as -I option to include jpeglib's headers
# CPPFLAGS += -I/home/fred/jpeglib

# Add linker options, such as -L option to include jpeglib's libraries
# LDFLAGS += -L/home/fred/jpeglib

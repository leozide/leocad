# LeoCAD configuration file
#
include version.mk

default: all

CC	:= gcc
CXX	:= g++

# (Add a -g for debugging)
CPPFLAGS += -O2 -Wall

# Add compile options, such as -I option to include jpeglib's headers
# CPPFLAGS += -I/home/fred/jpeglib

# Add linker options, such as -L option to include jpeglib's libraries
# LDFLAGS += -L/home/fred/jpeglib

### Linux configuration

ifeq ($(shell uname), Linux)

OS 	:= -DLC_LINUX
OSDIR 	:= linux

endif

### BeOS configuration

ifeq ($(shell uname), BeOS)

OS 	:= -DLC_BEOS
OSDIR 	:= beos

endif

.PHONY: config

### Automatic configuration

config:
	@echo "Automatic configuration"

	@echo "### LeoCAD configuration" > $(OSDIR)/config.mk
	@echo "### Auto-generated file, DO NOT EDIT" >> $(OSDIR)/config.mk
	@echo "" >> $(OSDIR)/config.mk

	@echo "//" > $(OSDIR)/config.h
	@echo "// LeoCAD configuration" >> $(OSDIR)/config.h
	@echo "//" >> $(OSDIR)/config.h
	@echo "// Auto-generated file, DO NOT EDIT" >> $(OSDIR)/config.h
	@echo "//" >> $(OSDIR)/config.h
	@echo "" >> $(OSDIR)/config.h
	@echo "#ifndef _CONFIG_H_" >> $(OSDIR)/config.h
	@echo "#define _CONFIG_H_" >> $(OSDIR)/config.h
	@echo "" >> $(OSDIR)/config.h

### Version information
	@echo "#define LC_VERSION_MAJOR $(MAJOR)" >> $(OSDIR)/config.h
	@echo "#define LC_VERSION_MINOR $(MINOR)" >> $(OSDIR)/config.h
	@echo "#define LC_VERSION_PATCH $(PATCHLVL)" >> $(OSDIR)/config.h
	@echo "#define LC_VERSION_OSNAME \"$(shell uname)\"" >> $(OSDIR)/config.h
	@echo "#define LC_VERSION \"$(MAJOR).$(MINOR).$(PATCHLVL)\"" >> $(OSDIR)/config.h
	@echo "" >> $(OSDIR)/config.h

### Check if the user has libjpeg installed
	@echo -n "Checking for jpeg support... "
	@echo "char jpeg_read_header();" > jpegtest.c
	@echo "int main() { jpeg_read_header(); return 0; }" >> jpegtest.c
	@if { (eval $(CC) jpegtest.c -ljpeg -o jpegtest $(CPPFLAGS) $(LDFLAGS)) 2> /dev/null; } && \
	  (test -s jpegtest); then  \
	  echo "ok"; \
	  echo "HAVE_JPEGLIB = yes" >> $(OSDIR)/config.mk; \
	  echo "#define LC_HAVE_JPEGLIB" >> $(OSDIR)/config.h; \
	else \
	  echo "no (libjpeg required)"; \
	  echo "HAVE_JPEGLIB = no" >> $(OSDIR)/config.mk; \
	  echo "#undef LC_HAVE_JPEGLIB" >> $(OSDIR)/config.h; \
	fi
	@rm -rf jpegtest.c jpegtest

### Check if the user has zlib installed
	@echo -n "Checking for zlib support... "
	@echo "char gzread();" > ztest.c
	@echo "int main() { gzread(); return 0; }" >> ztest.c
	@if { (eval $(CC) ztest.c -lz -o ztest $(CPPFLAGS) $(LDFLAGS)) 2> /dev/null; } && \
	  (test -s ztest); then  \
	  echo "ok"; \
	  echo "HAVE_ZLIB = yes" >> $(OSDIR)/config.mk; \
	  echo "#define LC_HAVE_ZLIB" >> $(OSDIR)/config.h; \
	else \
	  echo "no (zlib required)"; \
	  echo "HAVE_ZLIB = no" >> $(OSDIR)/config.mk; \
	  echo "#undef LC_HAVE_ZLIB" >> $(OSDIR)/config.h; \
	fi
	@rm -rf ztest.c ztest

### Check if the user has libjpeg installed
	@echo -n "Checking for png support... "
	@echo "char png_read_info();" > pngtest.c
	@echo "int main() { png_read_info(); return 0; }" >> pngtest.c
	@if { (eval $(CC) pngtest.c -lpng -o pngtest $(CPPFLAGS) $(LDFLAGS)) 2> /dev/null; } && \
	  (test -s pngtest); then  \
	  echo "ok"; \
	  echo "HAVE_PNGLIB = yes" >> $(OSDIR)/config.mk; \
	  echo "#define LC_HAVE_PNGLIB" >> $(OSDIR)/config.h; \
	else \
	  echo "no (libpng required)"; \
	  echo "HAVE_PNGLIB = no" >> $(OSDIR)/config.mk; \
	  echo "#undef LC_HAVE_PNGLIB" >> $(OSDIR)/config.h; \
	fi
	@rm -rf pngtest.c pngtest

	@echo "" >> $(OSDIR)/config.h
	@echo "#endif // _CONFIG_H_" >> $(OSDIR)/config.h

# LeoCAD configuration file
#
include version.mk

ERROR_SETTING=2> /dev/null

default: all

CC    := gcc
CXX   := g++
OSDIR := linux

# (Add a -g for debugging)
CPPFLAGS += -O2 -Wall

### FreeBSD configuration

ifeq ($(shell uname), FreeBSD)
CPPFLAGS += -L/usr/local/lib
endif

### Default directory

ifeq ($(PREFIX), )
PREFIX := /usr/local
endif

.PHONY: config config-help

config-help:
	@echo "This target attempts to detect your system settings and create $(OSDIR)/config.mk"
	@echo "Valid parameters and their default values are:"
	@echo "  PREFIX=/usr/local"
	@echo "  DESTDIR="

### Automatic configuration

#USE this with printf and a primitive type - printf "WIDTHTEST" "char" >conftest.c
CONFTEST="\#include <stdio.h>\nint main() { FILE *f=fopen(\"conftestval\", \"w\");\n\
	if (!f) return 1; fprintf(f, \"%%d\\\n\", (int)sizeof(%s)); return 0; }\n"

$(OSDIR)/config.mk: config.mk version.mk
	make config

config:
	@echo "Automatic configuration"

	@echo "### LeoCAD configuration" > $(OSDIR)/config.mk
	@echo "### Auto-generated file, DO NOT EDIT" >> $(OSDIR)/config.mk
	@echo "" >> $(OSDIR)/config.mk
	@echo "PREFIX := $(PREFIX)" >> $(OSDIR)/config.mk;
	@echo "DESTDIR := $(DESTDIR)" >> $(OSDIR)/config.mk;
	@echo "" >> $(OSDIR)/config.mk
	@echo "CPPFLAGS += -DLC_INSTALL_PREFIX=\\\"$(PREFIX)\\\"" >> $(OSDIR)/config.mk
	@echo "" >> $(OSDIR)/config.mk

#### Check if the user has GTK+ and GLIB installed.
	@echo -n "Checking if GLIB and GTK+ are installed... "
	@if (pkg-config --atleast-version=2.0.0 glib-2.0) && (pkg-config --atleast-version=2.0.0 gtk+-2.0); then \
	  echo "ok"; \
	  echo "CPPFLAGS += \$$(shell pkg-config gtk+-2.0 --cflags)" >> $(OSDIR)/config.mk; \
	  echo "LIBS += \$$(shell pkg-config gtk+-2.0 --libs)" >> $(OSDIR)/config.mk; \
	else \
	  echo "failed"; \
	  rm -rf $(OSDIR)/config.mk; \
	  exit 1; \
	fi

## Check if the user has OpenGL installed
	@echo -n "Checking for OpenGL support... "
	@echo "#include <GL/gl.h>" > gltest.c
	@echo "int main() { return 0; }" >> gltest.c
	@if { (eval $(CC) gltest.c -o gltest $(CPPFLAGS) $(LDFLAGS)); } && \
	  (test -s gltest); then  \
	  echo "ok"; \
	else \
	  rm -f gltest.c gltest; \
	  echo "failed"; \
	  rm -rf $(OSDIR)/config.mk; \
	  exit 1; \
	fi
	@rm -f gltest.c gltest

### Check if the user has zlib installed
	@echo -n "Checking for zlib support... "
	@echo "char gzread();" > ztest.c
	@echo "int main() { gzread(); return 0; }" >> ztest.c
	@if { (eval $(CC) ztest.c -lz -o ztest $(CPPFLAGS) $(LDFLAGS)); } && \
	  (test -s ztest); then  \
	  echo "ok"; \
	else \
	  rm -f ztest.c ztest \
	  echo "failed"; \
	  rm -rf $(OSDIR)/config.mk; \
	  exit 1; \
	fi
	@rm -f ztest.c ztest

## Check if the user has libjpeg installed
	@echo -n "Checking for jpeg support... "
	@echo "char jpeg_read_header();" > jpegtest.c
	@echo "int main() { jpeg_read_header(); return 0; }" >> jpegtest.c
	@if { (eval $(CC) jpegtest.c -ljpeg -o jpegtest $(CPPFLAGS) $(LDFLAGS)); } && \
	  (test -s jpegtest); then  \
	  echo "ok"; \
	  echo "" >> $(OSDIR)/config.mk; \
	  echo "HAVE_JPEGLIB = yes" >> $(OSDIR)/config.mk; \
	  echo "CPPFLAGS += -DLC_HAVE_JPEGLIB" >> $(OSDIR)/config.mk; \
	else \
	  echo "no (libjpeg optional)"; \
	  echo "HAVE_JPEGLIB = no" >> $(OSDIR)/config.mk; \
	fi
	@rm -f jpegtest.c jpegtest

### Check if the user has libpng installed
	@echo -n "Checking for png support... "
	@echo "char png_read_info();" > pngtest.c
	@echo "int main() { png_read_info(); return 0; }" >> pngtest.c
	@if { (eval $(CC) pngtest.c -lm -lz -lpng -o pngtest $(CPPFLAGS) $(LDFLAGS)); } && \
	  (test -s pngtest); then  \
	  echo "ok"; \
	  echo "" >> $(OSDIR)/config.mk; \
	  echo "HAVE_PNGLIB = yes" >> $(OSDIR)/config.mk; \
	  echo "CPPFLAGS += -DLC_HAVE_PNGLIB" >> $(OSDIR)/config.mk; \
	else \
	  echo "no (libpng optional)"; \
	  echo "HAVE_PNGLIB = no" >> $(OSDIR)/config.mk; \
	fi
	@rm -f pngtest.c pngtest


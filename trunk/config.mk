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

OS 	   := -DLC_LINUX
OSDIR 	   := linux
GTK_CONFIG := gtk-config

endif

### FreeBSD configuration

ifeq ($(shell uname), FreeBSD)

OS 	   := -DLC_LINUX
OSDIR 	   := linux
GTK_CONFIG := gtk12-config
CPPFLAGS   += -L/usr/local/lib

endif

### BeOS configuration

ifeq ($(shell uname), BeOS)

OS 	:= -DLC_BEOS
OSDIR 	:= beos

endif

### Default directory

ifeq ($(PREFIX), )
PREFIX := /usr/local
endif

.PHONY: config config-help

config-help:
	@echo "This target attempts to detect your system settings,"
	@echo "it will create $(OSDIR)/config.mk and $(OSDIR)/config.h"
	@echo "Valid parameters and their default values are:"
	@echo "  PREFIX=/usr/local"
	@echo "  DESTDIR="

### Automatic configuration

config:
	@echo "Automatic configuration"

	@echo "### LeoCAD configuration" > $(OSDIR)/config.mk
	@echo "### Auto-generated file, DO NOT EDIT" >> $(OSDIR)/config.mk
	@echo "" >> $(OSDIR)/config.mk
	@echo "PREFIX := $(PREFIX)" >> $(OSDIR)/config.mk;
	@echo "DESTDIR := $(DESTDIR)" >> $(OSDIR)/config.mk;
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
	@echo "#define LC_INSTALL_PREFIX \"$(PREFIX)\"" >> $(OSDIR)/config.h
	@echo "" >> $(OSDIR)/config.h

### Determine variable sizes
	@echo -n "checking size of char... "; \
	echo "#include <stdio.h>" > conftest.c; \
	echo "main() { FILE *f=fopen(\"conftestval\", \"w\");" >> conftest.c; \
	echo "if (!f) exit(1); fprintf(f, \"%d\\n\", sizeof(char)); exit(0); }" >> conftest.c; \
	if { (eval $(CC) conftest.c -o conftest) 2> /dev/null; } && \
	  (test -s conftest && (./conftest; exit) 2> /dev/null); then \
	  ac_cv_sizeof_char=`cat conftestval`; \
	  echo "$$ac_cv_sizeof_char"; \
	else \
	  echo "failed to get size of char"; \
	  ac_cv_sizeof_char=0; \
	fi; \
	echo "#define LC_SIZEOF_CHAR $$ac_cv_sizeof_char" >> $(OSDIR)/config.h; \
	rm -f conftest.c conftest conftestval; \
	\
	echo -n "checking size of short... "; \
	echo "#include <stdio.h>" > conftest.c; \
	echo "main() { FILE *f=fopen(\"conftestval\", \"w\");" >> conftest.c; \
	echo "if (!f) exit(1); fprintf(f, \"%d\\n\", sizeof(short)); exit(0); }" >> conftest.c; \
	if { (eval $(CC) conftest.c -o conftest) 2> /dev/null; } && \
	  (test -s conftest && (./conftest; exit) 2> /dev/null); then \
	  ac_cv_sizeof_short=`cat conftestval`; \
	  echo "$$ac_cv_sizeof_short"; \
	else \
	  echo "failed to get size of short"; \
	  ac_cv_sizeof_short=0; \
	fi; \
	echo "#define LC_SIZEOF_SHORT $$ac_cv_sizeof_short" >> $(OSDIR)/config.h; \
	rm -f conftest.c conftest conftestval; \
	\
	echo -n "checking size of long... "; \
	echo "#include <stdio.h>" > conftest.c; \
	echo "main() { FILE *f=fopen(\"conftestval\", \"w\");" >> conftest.c; \
	echo "if (!f) exit(1); fprintf(f, \"%d\\n\", sizeof(long)); exit(0); }" >> conftest.c; \
	if { (eval $(CC) conftest.c -o conftest) 2> /dev/null; } && \
	  (test -s conftest && (./conftest; exit) 2> /dev/null); then \
	  ac_cv_sizeof_long=`cat conftestval`; \
	  echo "$$ac_cv_sizeof_long"; \
	else \
	  echo "failed to get size of long"; \
	  ac_cv_sizeof_long=0; \
	fi; \
	echo "#define LC_SIZEOF_LONG $$ac_cv_sizeof_long" >> $(OSDIR)/config.h; \
	rm -f conftest.c conftest conftestval; \
	\
	echo -n "checking size of int... "; \
	echo "#include <stdio.h>" > conftest.c; \
	echo "main() { FILE *f=fopen(\"conftestval\", \"w\");" >> conftest.c; \
	echo "if (!f) exit(1); fprintf(f, \"%d\\n\", sizeof(int)); exit(0); }" >> conftest.c; \
	if { (eval $(CC) conftest.c -o conftest) 2> /dev/null; } && \
	  (test -s conftest && (./conftest; exit) 2> /dev/null); then \
	  ac_cv_sizeof_int=`cat conftestval`; \
	  echo "$$ac_cv_sizeof_int"; \
	else \
	  echo "failed to get size of int"; \
	  ac_cv_sizeof_int=0; \
	fi; \
	echo "#define LC_SIZEOF_INT $$ac_cv_sizeof_int" >> $(OSDIR)/config.h; \
	rm -f conftest.c conftest conftestval; \
	\
	echo -n "checking size of void *... "; \
	echo "#include <stdio.h>" > conftest.c; \
	echo "main() { FILE *f=fopen(\"conftestval\", \"w\");" >> conftest.c; \
	echo "if (!f) exit(1); fprintf(f, \"%d\\n\", sizeof(void *)); exit(0); }" >> conftest.c; \
	if { (eval $(CC) conftest.c -o conftest) 2> /dev/null; } && \
	  (test -s conftest && (./conftest; exit) 2> /dev/null); then \
	  ac_cv_sizeof_void_p=`cat conftestval`; \
	  echo "$$ac_cv_sizeof_void_p"; \
	else \
	  echo "failed to get size of void *"; \
	  ac_cv_sizeof_void_p=0; \
	fi; \
	echo "#define LC_SIZEOF_VOID_P $$ac_cv_sizeof_void_p" >> $(OSDIR)/config.h; \
	rm -f conftest.c conftest conftestval; \
	\
	echo -n "checking size of long long... "; \
	echo "#include <stdio.h>" > conftest.c; \
	echo "main() { FILE *f=fopen(\"conftestval\", \"w\");" >> conftest.c; \
	echo "if (!f) exit(1); fprintf(f, \"%d\\n\", sizeof(long long)); exit(0); }" >> conftest.c; \
	if { (eval $(CC) conftest.c -o conftest) 2> /dev/null; } && \
	  (test -s conftest && (./conftest; exit) 2> /dev/null); then \
	  ac_cv_sizeof_long_long=`cat conftestval`; \
	  echo "$$ac_cv_sizeof_long_long"; \
	else \
	  echo "failed to get size of long long"; \
	  ac_cv_sizeof_long_long=0; \
	fi; \
	echo "#define LC_SIZEOF_LONG_LONG $$ac_cv_sizeof_long_long" >> $(OSDIR)/config.h; \
	rm -f conftest.c conftest conftestval; \
	case 2 in \
	  $$ac_cv_sizeof_short)		lcint16=short;; \
	  $$ac_cv_sizeof_int)		lcint16=int;; \
	esac; \
	case 4 in \
	  $$ac_cv_sizeof_short)		lcint32=short;; \
	  $$ac_cv_sizeof_int)		lcint32=int;; \
	  $$ac_cv_sizeof_long)		lcint32=long;; \
	esac; \
	echo "" >> $(OSDIR)/config.h; \
	echo "typedef signed char lcint8;" >> $(OSDIR)/config.h; \
	echo "typedef unsigned char lcuint8;" >> $(OSDIR)/config.h; \
	if test -n "$$lcint16"; then \
	  echo "typedef signed $$lcint16 lcint16;" >> $(OSDIR)/config.h; \
	  echo "typedef unsigned $$lcint16 lcuint16;" >> $(OSDIR)/config.h; \
	else \
	  echo "#error need to define lcint16 and lcuint16" >> $(OSDIR)/config.h; \
	fi; \
	if test -n "$$lcint32"; then \
	echo "typedef signed $$lcint32 lcint32;" >> $(OSDIR)/config.h; \
	echo "typedef unsigned $$lcint32 lcuint32;" >> $(OSDIR)/config.h; \
	else \
	  echo "#error need to define lcint32 and lcuint32" >> $(OSDIR)/config.h; \
	fi; \
	echo "" >> $(OSDIR)/config.h

### Check if machine is little or big endian
	@echo -n "Determining endianess... "
	@echo "main () { union { long l; char c[sizeof (long)]; } u;" > endiantest.c
	@echo "u.l = 1; exit (u.c[sizeof (long) - 1] == 1); }" >> endiantest.c
	@if { (eval $(CC) endiantest.c -o endiantest) 2> /dev/null; } && \
	  (test -s endiantest && (./endiantest; exit) 2> /dev/null); then \
	  echo "little endian"; \
	  echo "#define LC_LITTLE_ENDIAN" >> $(OSDIR)/config.h; \
	else \
	  echo "big endian"; \
	  echo "#define LC_BIG_ENDIAN" >> $(OSDIR)/config.h; \
	fi
	@rm -f endiantest.c endiantest

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
	@rm -f jpegtest.c jpegtest

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
	@rm -f ztest.c ztest

### Check if the user has libpng installed
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
	@rm -f pngtest.c pngtest

	@echo "" >> $(OSDIR)/config.h
	@echo "#endif // _CONFIG_H_" >> $(OSDIR)/config.h

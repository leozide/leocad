# LeoCAD configuration file
#
include version.mk

ERROR_SETTING=2> /dev/null

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
TEST_GTK   := 1

endif

### FreeBSD configuration

ifeq ($(shell uname), FreeBSD)

OS 	   := -DLC_LINUX
OSDIR 	   := linux
TEST_GTK   := 1
CPPFLAGS   += -L/usr/local/lib

endif

### Macintosh configuration

ifeq ($(shell uname), Darwin)

OS         := -DLC_MACOSX
OSDIR      := macosx
TEST_GTK   := 0
CPPFLAGS   += -funsigned-char -I/Developer/Headers/FlatCarbon/
LDFLAGS    += -framework AGL -framework Carbon

# The directory tree for the generated bundle
BUNDLEDIR  = bin/LeoCAD.app
CONTDIR    = $(BUNDLEDIR)/Contents
BINDIR     = $(CONTDIR)/MacOS
RSRCDIR    = $(CONTDIR)/Resources
EN_RSRCDIR = $(RSRCDIR)/English.lproj

NIBS = main.nib

all: bundletree exec bnibs pkginfo infoplist infopliststr

bundletree: $(BINDIR) $(BUNDLEDIR) $(CONTDIR) $(RSRCDIR) $(EN_RSRCDIR)

$(BUNDLEDIR):
	mkdir bin
	mkdir $(BUNDLEDIR)

$(CONTDIR): $(BUNDLEDIR)
	mkdir $(CONTDIR)

$(BINDIR): $(CONTDIR)
	mkdir $(BINDIR)

$(RSRCDIR): $(CONTDIR)
	mkdir $(RSRCDIR)

$(EN_RSRCDIR): $(RSRCDIR)
	mkdir $(EN_RSRCDIR)

# The binary executable
exec: $(BINDIR)/leocad

$(BINDIR)/leocad: bin/leocad
	cp bin/leocad $(BINDIR)

# The nib's
BNIBS = $(NIBS:%=$(EN_RSRCDIR)/%)

bnibs: $(BNIBS)

$(BNIBS) : $(EN_RSRCDIR)/%: macosx/English.lproj/%
	rm -rf $@
	cp -R $< $@
	touch $@

# Info.plist
infoplist : $(CONTDIR)/Info.plist

$(CONTDIR)/Info.plist : macosx/Info.plist
	cp macosx/Info.plist $@

# PkgInfo
pkginfo : $(CONTDIR)/PkgInfo

$(CONTDIR)/PkgInfo : macosx/PkgInfo
	cp macosx/PkgInfo $@

# InfoPlist.strings - this goes inside English.lproj
infopliststr : $(EN_RSRCDIR)/InfoPlist.strings

$(EN_RSRCDIR)/InfoPlist.strings: macosx/English.lproj/InfoPlist.strings
	cp $< $@

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

#USE this with printf and a primitive type - printf "WIDTHTEST" "char" >conftest.c
CONFTEST="\#include <stdio.h>\nint main() { FILE *f=fopen(\"conftestval\", \"w\");\n\
	if (!f) return 1; fprintf(f, \"%%d\\\n\", (int)sizeof(%s)); return 0; }\n"

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
	@echo "#define LC_VERSION_TEXT \"$(MAJOR).$(MINOR).$(PATCHLVL)\"" >> $(OSDIR)/config.h
	@echo "#define LC_VERSION_TAG \"$(VERSIONTAG)\"" >> $(OSDIR)/config.h
	@echo "#define LC_INSTALL_PREFIX \"$(PREFIX)\"" >> $(OSDIR)/config.h
	@echo "" >> $(OSDIR)/config.h

### Determine variable sizes
	@echo -n "checking size of char... "; \
	printf ${CONFTEST} "char" > conftest.c; \
	if { (eval $(CC) conftest.c -o conftest); } && \
	  (test -s conftest && (./conftest; exit)); then \
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
	printf ${CONFTEST} "short" > conftest.c; \
	if { (eval $(CC) conftest.c -o conftest); } && \
	  (test -s conftest && (./conftest; exit)); then \
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
	printf ${CONFTEST} "long" > conftest.c; \
	if { (eval $(CC) conftest.c -o conftest); } && \
	  (test -s conftest && (./conftest; exit)); then \
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
	printf ${CONFTEST} "int" > conftest.c; \
	if { (eval $(CC) conftest.c -o conftest); } && \
	  (test -s conftest && (./conftest; exit)); then \
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
	printf ${CONFTEST} "void *" > conftest.c; \
	if { (eval $(CC) conftest.c -o conftest); } && \
	  (test -s conftest && (./conftest; exit)); then \
	  ac_cv_sizeof_void_p=`cat conftestval`; \
	  echo "$$ac_cv_sizeof_void_p"; \
	else \
	  echo "failed to get size of void *"; \
	  ac_cv_sizeof_void_p=0; \
	fi; \
	echo "#define LC_SIZEOF_VOID_P $$ac_cv_sizeof_void_p" >> $(OSDIR)/config.h; \
	if test "$$ac_cv_sizeof_void_p" -eq "8"; then \
	  echo "#define LC_POINTER_TO_INT(p) ((lcint32)(lcint64)(p))" >> $(OSDIR)/config.h; \
	else \
	  echo "#define LC_POINTER_TO_INT(p) ((lcint32)(p))" >> $(OSDIR)/config.h; \
	fi; \
	rm -f conftest.c conftest conftestval; \
	\
	echo -n "checking size of long long... "; \
	printf ${CONFTEST} "long long" > conftest.c; \
	if { (eval $(CC) conftest.c -o conftest); } && \
	  (test -s conftest && (./conftest; exit)); then \
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
	case 8 in \
	  $$ac_cv_sizeof_long_long)	lcint64="long long";; \
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
	if test -n "$$lcint64"; then \
	echo "typedef signed $$lcint64 lcint64;" >> $(OSDIR)/config.h; \
	echo "typedef unsigned $$lcint64 lcuint64;" >> $(OSDIR)/config.h; \
	else \
	  echo "#error need to define lcint64 and lcuint64" >> $(OSDIR)/config.h; \
	fi; \
	echo "" >> $(OSDIR)/config.h

### Check if machine is little or big endian
	@echo -n "Determining endianess... "
	@echo "int main () { union { long l; char c[sizeof (long)]; } u;" > endiantest.c
	@echo "u.l = 1; return (u.c[sizeof (long) - 1] == 1); }" >> endiantest.c
	@if { (eval $(CC) endiantest.c -o endiantest); } && \
	  (test -s endiantest && (./endiantest; exit)); then \
	  echo "little endian"; \
	  echo "#define LC_LITTLE_ENDIAN" >> $(OSDIR)/config.h; \
	  echo "#define LCUINT16(val) val" >> $(OSDIR)/config.h; \
	  echo "#define LCUINT32(val) val" >> $(OSDIR)/config.h; \
	  echo "#define LCINT16(val) val" >> $(OSDIR)/config.h; \
	  echo "#define LCINT32(val) val" >> $(OSDIR)/config.h; \
	  echo "#define LCFLOAT(val) val" >> $(OSDIR)/config.h; \
	else \
	  echo "big endian"; \
	  echo "#define LC_BIG_ENDIAN" >> $(OSDIR)/config.h; \
	  echo "#define LCUINT16(val) ((lcuint16) ( \\" >> $(OSDIR)/config.h; \
	  echo "    (((lcuint16) (val) & (lcuint16) 0x00ffU) << 8) | \\" >> $(OSDIR)/config.h; \
	  echo "    (((lcuint16) (val) & (lcuint16) 0xff00U) >> 8)))" >> $(OSDIR)/config.h; \
	  echo "#define LCUINT32(val) ((lcuint32) ( \\" >> $(OSDIR)/config.h; \
	  echo "    (((lcuint32) (val) & (lcuint32) 0x000000ffU) << 24) | \\" >> $(OSDIR)/config.h; \
	  echo "    (((lcuint32) (val) & (lcuint32) 0x0000ff00U) <<  8) | \\" >> $(OSDIR)/config.h; \
	  echo "    (((lcuint32) (val) & (lcuint32) 0x00ff0000U) >>  8) | \\" >> $(OSDIR)/config.h; \
	  echo "    (((lcuint32) (val) & (lcuint32) 0xff000000U) >> 24)))" >> $(OSDIR)/config.h; \
	  echo "#define LCINT16(val) ((lcint16)LCUINT16(val))" >> $(OSDIR)/config.h; \
	  echo "#define LCINT32(val) ((lcint32)LCUINT32(val))" >> $(OSDIR)/config.h; \
	  echo -e "inline float LCFLOAT (float l)\n{" >> $(OSDIR)/config.h; \
	  echo -e "  union { unsigned char b[4]; float f; } in, out;\n" >> $(OSDIR)/config.h; \
	  echo -e "  in.f = l;\n  out.b[0] = in.b[3];\n  out.b[1] = in.b[2];" >> $(OSDIR)/config.h; \
	  echo -e "  out.b[2] = in.b[1];\n  out.b[3] = in.b[0];\n" >> $(OSDIR)/config.h; \
	  echo -e "  return out.f;\n}" >> $(OSDIR)/config.h; \
	fi; \
	echo "" >> $(OSDIR)/config.h
	@rm -f endiantest.c endiantest

#### Check if the user has GTK+ and GLIB installed.
ifeq ($(TEST_GTK), 1)
	@echo -n "Checking if GLIB and GTK+ are installed... "
	@if (pkg-config --atleast-version=2.0.0 glib-2.0) && (pkg-config --atleast-version=2.0.0 gtk+-2.0); then \
	  echo "ok"; \
	  echo "CFLAGS += \$$(shell pkg-config gtk+-2.0 --cflags)" >> $(OSDIR)/config.mk; \
	  echo "CXXFLAGS += \$$(shell pkg-config gtk+-2.0 --cflags)" >> $(OSDIR)/config.mk; \
	  echo "LIBS += \$$(shell pkg-config gtk+-2.0 --libs)" >> $(OSDIR)/config.mk; \
	else \
	  echo "failed"; \
	  rm -rf $(OSDIR)/config.mk $(OSDIR)/config.h; \
	  exit 1; \
	fi
endif

## Check if the user has libjpeg installed
	@echo -n "Checking for jpeg support... "
	@echo "char jpeg_read_header();" > jpegtest.c
	@echo "int main() { jpeg_read_header(); return 0; }" >> jpegtest.c
	@if { (eval $(CC) jpegtest.c -ljpeg -o jpegtest $(CPPFLAGS) $(LDFLAGS)); } && \
	  (test -s jpegtest); then  \
	  echo "ok"; \
	  echo "HAVE_JPEGLIB = yes" >> $(OSDIR)/config.mk; \
	  echo "#define LC_HAVE_JPEGLIB" >> $(OSDIR)/config.h; \
	else \
	  echo "no (libjpeg optional)"; \
	  echo "HAVE_JPEGLIB = no" >> $(OSDIR)/config.mk; \
	  echo "#undef LC_HAVE_JPEGLIB" >> $(OSDIR)/config.h; \
	fi
	@rm -f jpegtest.c jpegtest

### Check if the user has zlib installed
	@echo -n "Checking for zlib support... "
	@echo "char gzread();" > ztest.c
	@echo "int main() { gzread(); return 0; }" >> ztest.c
	@if { (eval $(CC) ztest.c -lz -o ztest $(CPPFLAGS) $(LDFLAGS)); } && \
	  (test -s ztest); then  \
	  echo "ok"; \
	  echo "HAVE_ZLIB = yes" >> $(OSDIR)/config.mk; \
	  echo "#define LC_HAVE_ZLIB" >> $(OSDIR)/config.h; \
	else \
	  echo "no (zlib optional)"; \
	  echo "HAVE_ZLIB = no" >> $(OSDIR)/config.mk; \
	  echo "#undef LC_HAVE_ZLIB" >> $(OSDIR)/config.h; \
	fi
	@rm -f ztest.c ztest

### Check if the user has libpng installed
	@echo -n "Checking for png support... "
	@echo "char png_read_info();" > pngtest.c
	@echo "int main() { png_read_info(); return 0; }" >> pngtest.c
	@if { (eval $(CC) pngtest.c -lm -lz -lpng -o pngtest $(CPPFLAGS) $(LDFLAGS)); } && \
	  (test -s pngtest); then  \
	  echo "ok"; \
	  echo "HAVE_PNGLIB = yes" >> $(OSDIR)/config.mk; \
	  echo "#define LC_HAVE_PNGLIB" >> $(OSDIR)/config.h; \
	else \
	  echo "no (libpng optional)"; \
	  echo "HAVE_PNGLIB = no" >> $(OSDIR)/config.mk; \
	  echo "#undef LC_HAVE_PNGLIB" >> $(OSDIR)/config.h; \
	fi
	@rm -f pngtest.c pngtest

	@echo "" >> $(OSDIR)/config.h
	@echo "#endif // _CONFIG_H_" >> $(OSDIR)/config.h

### ALL CONFIGURATION SHOULD BE IN CONFIG.MK, NOT HERE
include config.mk
-include $(OSDIR)/config.mk

### Module directories
MODULES := $(OSDIR) common

### look for include files in
###   each of the modules
CPPFLAGS += $(patsubst %,-I%,$(MODULES)) $(OS)
CPPFLAGS += -g

### extra libraries if required
LIBS :=

### each module will add to this
SRC :=

BIN := bin/leocad

### include the description for
###   each module
include $(patsubst %,%/module.mk,$(MODULES))

### determine the object files
OBJ := \
  $(patsubst %.c,%.o,$(filter %.c,$(SRC))) \
  $(patsubst %.cpp,%.o,$(filter %.cpp,$(SRC)))

### use c++ for all files (the gtkglarea .c files will be removed shortly)
CFLAGS += -x c++

### link the program
.PHONY: all static

all: $(BIN)

static: bin/leocad.static

bin/leocad: $(OBJ) bin
	$(CC) -o $@ $(OBJ) $(LIBS)

bin/leocad.static: $(OBJ) bin
	$(CC) -static -o $@ $(OBJ) $(LIBS)

bin:
	mkdir bin

### include the C/C++ include
###   dependencies
ifeq ($(findstring $(MAKECMDGOALS), config clean veryclean spotless), )
-include $(OBJ:.o=.d)
endif

### calculate C/C++ include
###   dependencies
%.d: %.c
	@[ -s $(OSDIR)/config.h ] || $(MAKE) config
	@./depend.sh $@ $(@D) $(CC) $(CFLAGS) $(CPPFLAGS) -w $<
	@[ -s $@ ] || rm -f $@

%.d: %.cpp
	@[ -s $(OSDIR)/config.h ] || $(MAKE) config
	@./depend.sh $@ $(@D) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -w $<
	@[ -s $@ ] || rm -f $@

### Various cleaning functions
.PHONY: clean veryclean spotless all

clean:
	find $(MODULES) -name \*.o | xargs rm -f

veryclean: clean
	find $(MODULES) -name \*.d | xargs rm -f
	rm -rf bin

spotless: veryclean
	rm -rf arch $(OSDIR)/config.mk $(OSDIR)/config.h


### dependency stuff is done automatically, so these do nothing.
.PHONY: dep depend


### Help function
.PHONY: help

help:
	@echo   'Possible Targets are:'
	@echo   '       help (this is it)'
	@echo   '       all'
	@echo   '       install'
	@echo   '       binary'
	@echo   '       source'
	@echo   '       (binary and source can be called as'
	@echo   '        a -zip or -tgz variants)'
	@echo   '       clean'
	@echo   '       veryclean'
	@echo   '       spotless'
	@echo

###  Rules to make various packaging
.PHONY: binary binary-tgz source-zip source-tgz source install

arch:
	mkdir arch

install: $(BIN)
	install -c -m 0755 $(BIN) $(PREFIX)/bin
	install -c -m 0644 docs/leocad.1 $(PREFIX)/man/man1

binary: binary-zip binary-tgz

binary-zip: arch/leocad-$(VERSION)-linux.zip

binary-tgz: arch/leocad-$(VERSION)-linux.tgz

source: source-tgz source-zip

source-tgz: arch/leocad-$(VERSION)-src.tgz

source-zip: arch/leocad-$(VERSION)-src.zip

### Create a directory with the files needed for a binary package
package-dir: arch all
	mkdir leocad-$(VERSION)
	cp bin/leocad leocad-$(VERSION)
	cp CREDITS.txt leocad-$(VERSION)/CREDITS
	cp README.txt leocad-$(VERSION)/README
	cp docs/INSTALL.txt leocad-$(VERSION)/INSTALL
	cp docs/LINUX.txt leocad-$(VERSION)/LINUX
	cp docs/leocad.1 leocad-$(VERSION)

arch/leocad-$(VERSION)-linux.zip: package-dir
	rm -f $@
	zip -r $@ leocad-$(VERSION)
	rm -rf leocad-$(VERSION)

arch/leocad-$(VERSION)-linux.tgz: package-dir
	rm -f $@
	tar -cvzf $@ leocad-$(VERSION)
	rm -rf leocad-$(VERSION)

arch/leocad-$(VERSION)-src.tgz: arch veryclean
	rm -f $@
	( cd .. ; tar --exclude=leocad/arch/\* --exclude=CVS \
	-cvzf leocad/$@ leocad )

arch/leocad-$(VERSION)-src.zip: arch veryclean
	rm -f $@
	( cd .. ; zip -r leocad/$@ leocad -x '*/arch/*' -x '*/CVS/*' )


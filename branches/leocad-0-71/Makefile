### ALL CONFIGURATION SHOULD IN CONFIG.MK, NOT HERE
include config.mk

### Module directories
MODULES := $(OSDIR) common

### look for include files in
###   each of the modules
CPPFLAGS += $(patsubst %,-I%,$(MODULES))
CPPFLAGS += $(OS) -DVERSION=$(VERSION)

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


### link the program
.PHONY: all

all: $(BIN)

bin/leocad: $(OBJ) bin
	$(CC) -o $@ $(OBJ) $(LIBS)

bin:
	mkdir bin

### include the C/C++ include
###   dependencies
include $(OBJ:.o=.d)

### calculate C/C++ include
###   dependencies
%.d: %.c
	./depend.sh $(CC) $(CFLAGS) $(CPPFLAGS) $< > $@
	[ -s $@ ] || rm -f $@

%.d: %.cpp
	./depend.sh $(CXX) $(CXXFLAGS) $(CPPFLAGS) $< > $@
	[ -s $@ ] || rm -f $@


### Various cleaning functions
.PHONY: clean veryclean spotless all

clean:
	find $(MODULES) -name \*.o | xargs rm -f

veryclean: clean
	find $(MODULES) -name \*.d | xargs rm -f
	rm -rf bin

spotless: veryclean
	rm -rf arch


### dependency stuff is done automatically, so these do nothing.
.PHONY: dep depend


### Help function
.PHONY: help

help:
	@echo   'Possible Targets are:'
	@echo   '       help (this is it)'
	@echo   '       all'
	@echo   '       binary'
	@echo   '       source'
	@echo   '       (binary and source can be called as'
	@echo   '        a -zip or -tgz variants)'
	@echo   '       clean'
	@echo   '       veryclean'
	@echo   '       spotless'
	@echo

###  Rules to make various packaging
.PHONY: binary binary-tgz source-zip source-tgz source

arch:
	mkdir arch

binary: binary-zip binary-tgz

binary-zip: arch/leocad-$(VERSION)-linux.zip

binary-tgz: arch/leocad-$(VERSION)-linux.tgz

source: source-tgz source-zip

source-tgz: arch/leocad-$(VERSION)-src.tgz

source-zip: arch/leocad-$(VERSION)-src.zip



arch/leocad-$(VERSION)-linux.zip: arch all
	rm -f $@
	zip -r $@ bin docs examples

arch/leocad-$(VERSION)-linux.tgz: arch all
	rm -f $@
	tar cvzf $@ bin docs examples

arch/leocad-$(VERSION)-src.tgz: arch veryclean
	rm -f $@
	( cd .. ; tar --exclude=leocad/arch/\* -cvzf leocad/$@ leocad )

arch/leocad-$(VERSION)-src.zip: arch veryclean
	rm -f $@
	( cd .. ; zip -r leocad/$@ leocad -x leocad/arch/\* )


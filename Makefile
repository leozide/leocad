### ALL CONFIGURATION SHOULD BE IN CONFIG.MK, NOT HERE
include config.mk

### Module directories
MODULES := $(OSDIR) common

### Look for include files in each of the modules
CPPFLAGS += $(patsubst %,-I%,$(MODULES))
CPPFLAGS += -g -Wextra -Wall -Wno-unused-parameter -Wshadow

### Extra libraries if required
LIBS :=

### Each module will add to this
SRC :=

BIN := bin/leocad
OBJDIR := obj

ifeq ($(findstring $(MAKECMDGOALS), help config-help config clean veryclean source-tgz source-zip), )
-include $(OSDIR)/config.mk
endif

### Include the description for each module
include $(patsubst %,%/module.mk,$(MODULES))

### Determine the object files
OBJ := $(patsubst %.cpp,$(OBJDIR)/%.o,$(filter %.cpp,$(SRC)))

### Link the program
.PHONY: all static

all: $(BIN)

static: bin/leocad.static

bin/leocad: $(OBJ) bin Makefile
	@echo Linking $@
	@$(CXX) -o $@ $(OBJ) $(LIBS) $(LDFLAGS)

bin/leocad.static: $(OBJ) bin Makefile
	$(CXX) -static -o $@ $(OBJ) $(LIBS) $(LDFLAGS)

bin:
	@mkdir bin

obj:
	@mkdir $(OBJDIR) $(addprefix $(OBJDIR)/,$(MODULES))

### Include the C/C++ include dependencies
ifeq ($(findstring $(MAKECMDGOALS), help config-help config clean veryclean source-tgz source-zip), )
-include $(OBJ:.o=.d)
endif

### Calculate C/C++ include dependencies
$(OBJDIR)/%.d: %.cpp obj $(OSDIR)/config.mk
	@$(CXX) -MM -MT '$(patsubst %.d,%.o, $@)' $(CXXFLAGS) $(CPPFLAGS) -w $< > $@
	@[ -s $@ ] || rm -f $@

### Main compiler rule
$(OBJDIR)/%.o: %.cpp obj $(OSDIR)/config.mk
	@echo $<
	@$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o '$(patsubst %.cpp,%.o, $@)' $<
	@[ -s $@ ] || rm -f $@

### Various cleaning functions
.PHONY: clean distclean veryclean spotless all

clean:
	@[ ! -d $(OBJDIR) ] || find $(OBJDIR) -name \*.o | xargs rm -f

veryclean: clean
	@rm -rf $(OBJDIR)
	@rm -rf bin
	@rm -rf arch $(OSDIR)/config.mk

distclean: veryclean

### Dependency stuff is done automatically, so these do nothing.
.PHONY: dep depend


### Help function
.PHONY: help

help:
	@echo   'Possible Targets are:'
	@echo   '       help (this is it)'
	@echo   '       all'
	@echo   '       install'
	@echo   '       uninstall'
	@echo   '       binary'
	@echo   '       source'
	@echo   '       (binary and source can be called as'
	@echo   '        a -zip or -tgz variants)'
	@echo   '       clean'
	@echo   '       veryclean'
	@echo

###  Rules to make various packaging
.PHONY: binary binary-tgz source-zip source-tgz source install uninstall

arch:
	mkdir arch

desktop: obj
	@echo "[Desktop Entry]" > $(OBJDIR)/leocad.desktop
	@echo "Version=1.0" >> $(OBJDIR)/leocad.desktop
	@echo "Name=LeoCAD" >> $(OBJDIR)/leocad.desktop
	@echo "Comment=Create virtual LEGO models" >> $(OBJDIR)/leocad.desktop
	@echo "Comment[eo]=Kreu virtualajn LEGO-ajn modelojn" >> $(OBJDIR)/leocad.desktop
	@echo "Comment[it]=Crea modelli LEGO virtuali" >> $(OBJDIR)/leocad.desktop
	@echo "Comment[nb]=Lag virtuelle LEGO-modeller" >> $(OBJDIR)/leocad.desktop
	@echo "Comment[pt_BR]=Criar modelos virtuais de LEGO" >> $(OBJDIR)/leocad.desktop
	@echo "Exec=$(PREFIX)/bin/leocad %f" >> $(OBJDIR)/leocad.desktop
	@echo "Terminal=false" >> $(OBJDIR)/leocad.desktop
	@echo "Type=Application" >> $(OBJDIR)/leocad.desktop
	@echo "Icon=$(PREFIX)/share/pixmaps/leocad.svg" >> $(OBJDIR)/leocad.desktop
	@echo "MimeType=application/vnd.leocad;application/x-ldraw;application/x-multi-part-ldraw;application/x-ldlite;" >> $(OBJDIR)/leocad.desktop
	@echo "Categories=Graphics;3DGraphics;Education;" >> $(OBJDIR)/leocad.desktop

install: $(BIN) install-data install-update
uninstall: uninstall-data install-update

install-data: desktop
	@install -d $(DESTDIR)$(PREFIX)/bin
	@install -c -m 0755 $(BIN) $(DESTDIR)$(PREFIX)/bin/
	@install -d $(DESTDIR)$(PREFIX)/share/man/man1
	@install -c -m 0644 docs/leocad.1 $(DESTDIR)$(PREFIX)/share/man/man1/
	@install -d $(DESTDIR)$(PREFIX)/share/leocad
	@install -c -m 0644 tools/icon/icon128.png $(DESTDIR)$(PREFIX)/share/leocad/icon.png
	@install -d $(DESTDIR)$(PREFIX)/share/applications
	@install -c -m 0644 $(OBJDIR)/leocad.desktop $(DESTDIR)$(PREFIX)/share/applications/
	@install -d $(DESTDIR)$(PREFIX)/share/pixmaps
	@install -c -m 0644 tools/icon/icon.svg $(DESTDIR)$(PREFIX)/share/pixmaps/leocad.svg
	@install -d $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/mimetypes/
	@rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/mimetypes/application-vnd.leocad.svg
	@ln -s $(PREFIX)/share/pixmaps/leocad.svg $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/mimetypes/application-vnd.leocad.svg
	@install -d $(DESTDIR)$(PREFIX)/share/mime/packages
	@install -c -m 0644 linux/leocad-mime.xml $(DESTDIR)$(PREFIX)/share/mime/packages/

uninstall-data:
	@rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN)
	@rm -f $(DESTDIR)$(PREFIX)/share/man/man1/leocad.1
	@rm -f $(DESTDIR)$(PREFIX)/share/leocad/icon.png
	@rm -f $(DESTDIR)$(PREFIX)/share/applications/leocad.desktop
	@rm -f $(DESTDIR)$(PREFIX)/share/pixmaps/leocad.svg
	@rm -f $(DESTDIR)$(PREFIX)/share/icons/hicolor/scalable/mimetypes/application-vnd.leocad.svg
	@rm -f $(DESTDIR)$(PREFIX)/share/mime/packages/leocad-mime.xml

install-update:
	@if test -z "$(DESTDIR)"; then \
		if which gtk-update-icon-cache>/dev/null 2>&1; then \
			gtk-update-icon-cache -q -f -t $(DESTDIR)$(PREFIX)/share/icons/hicolor; \
		fi; \
		if which update-mime-database>/dev/null 2>&1; then \
			update-mime-database $(DESTDIR)$(PREFIX)/share/mime/; \
		fi; \
		if which update-desktop-database>/dev/null 2>&1; then \
			update-desktop-database; \
		fi; \
	fi

binary: binary-zip binary-tgz

binary-zip: arch/leocad-linux.zip

binary-tgz: arch/leocad-linux.tgz

source: source-tgz source-zip

source-tgz: arch/leocad-src.tgz

source-zip: arch/leocad-src.zip

### Create a directory with the files needed for a binary package
package-dir: arch all
	mkdir leocad
	cp bin/leocad leocad
	cp CREDITS.txt leocad/CREDITS
	cp README.txt leocad/README
	cp docs/INSTALL.txt leocad/INSTALL
	cp docs/LINUX.txt leocad/LINUX
	cp docs/leocad.1 leocad

arch/leocad-linux.zip: package-dir
	rm -f $@
	zip -r $@ leocad
	rm -rf leocad

arch/leocad-linux.tgz: package-dir
	rm -f $@
	tar -cvzf $@ leocad
	rm -rf leocad

arch/leocad-src.tgz: veryclean arch
	rm -f $@
	( cd .. ; tar --exclude=leocad/arch/\* --exclude=.svn -cvzf leocad/$@ leocad )

arch/leocad-src.zip: veryclean arch
	rm -f $@
	( cd .. ; zip -r leocad/$@ leocad -x '*/arch/*' -x '*/.svn/*' -x '*~' -x '*/core' -x '*/.#*')


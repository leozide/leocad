SRC += linux/profile.cpp linux/pixmenu.c \
       linux/dialogs.cpp linux/dlgpiece.cpp linux/dlgfile.cpp \
       linux/gtktools.cpp linux/main.cpp linux/menu.cpp \
       linux/system.cpp linux/toolbar.cpp linux/gtkmisc.cpp \
       linux/linux_gl.cpp linux/basewnd.cpp linux/glwindow.cpp

CFLAGS += $(shell $(GTK_CONFIG) --cflags)
CXXFLAGS += $(shell $(GTK_CONFIG) --cflags)
LIBS += $(shell $(GTK_CONFIG) --libs)


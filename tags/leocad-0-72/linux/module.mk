SRC += linux/custom.cpp linux/gdkgl.c linux/gtkglarea.c \
       linux/dialogs.cpp linux/dlgpiece.cpp linux/dlgfile.cpp \
       linux/gtktools.cpp linux/main.cpp linux/menu.cpp \
       linux/system.cpp linux/toolbar.cpp

CFLAGS += $(shell gtk-config --cflags)
CXXFLAGS += $(shell gtk-config --cflags)
LIBS += $(shell gtk-config --libs)


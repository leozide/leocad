SRC += linux/custom.cpp linux/gdkgl.c linux/gtkglarea.c linux/dialogs.cpp linux/gtktools.cpp linux/main.cpp linux/menu.cpp linux/system.cpp linux/toolbar.cpp

CFLAGS += `gtk-config --cflags`
CXXFLAGS += `gtk-config --cflags`
LIBS += `gtk-config --libs`


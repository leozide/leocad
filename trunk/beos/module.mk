SRC += beos/beos_gl.cpp beos/main.cpp beos/system.cpp

CFLAGS += -Ibeos/jpeglib
CXXFLAGS += -Ibeos/jpeglib
LIBS += -Lbeos/jpeglib


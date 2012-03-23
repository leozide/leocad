SRC += common/algebra.cpp common/camera.cpp common/console.cpp common/curve.cpp common/lc_file.cpp \
       common/globals.cpp common/group.cpp common/image.cpp common/im_bmp.cpp common/im_gif.cpp \
       common/lc_application.cpp common/library.cpp common/light.cpp common/mainwnd.cpp \
       common/matrix.cpp common/message.cpp common/minifig.cpp common/object.cpp common/opengl.cpp \
       common/piece.cpp common/pieceinf.cpp common/preview.cpp common/project.cpp common/quant.cpp \
       common/str.cpp common/terrain.cpp common/texfont.cpp common/texture.cpp common/tr.cpp \
       common/vector.cpp common/view.cpp

ifeq ($(HAVE_JPEGLIB), yes)
	LIBS += -ljpeg
	SRC += common/im_jpg.cpp
endif

ifeq ($(HAVE_ZLIB), yes)
ifeq ($(HAVE_PNGLIB), yes)
	LIBS += -lpng -lz
	SRC += common/im_png.cpp
endif
endif

LIBS += -lm


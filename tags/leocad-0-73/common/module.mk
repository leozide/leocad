SRC += common/camera.cpp common/curve.cpp common/file.cpp common/globals.cpp common/group.cpp common/image.cpp common/im_bmp.cpp common/im_gif.cpp common/library.cpp common/light.cpp common/matrix.cpp common/message.cpp common/minifig.cpp common/object.cpp common/opengl.cpp common/piece.cpp common/pieceinf.cpp common/project.cpp common/quant.cpp common/str.cpp common/terrain.cpp common/texture.cpp common/tr.cpp common/vector.cpp

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


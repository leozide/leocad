SRC += common/camera.cpp common/console.cpp common/curve.cpp common/lc_colors.cpp common/lc_file.cpp \
       common/lc_mesh.cpp common/globals.cpp common/group.cpp common/image.cpp common/im_bmp.cpp common/im_gif.cpp \
       common/lc_application.cpp common/lc_library.cpp common/light.cpp common/mainwnd.cpp \
       common/message.cpp common/minifig.cpp common/object.cpp common/opengl.cpp \
       common/piece.cpp common/pieceinf.cpp common/preview.cpp common/project.cpp common/quant.cpp \
       common/str.cpp common/terrain.cpp common/texfont.cpp common/lc_texture.cpp common/tr.cpp \
       common/view.cpp common/lc_zipfile.cpp

ifeq ($(HAVE_JPEGLIB), yes)
	LIBS += -ljpeg
	SRC += common/im_jpg.cpp
endif

ifeq ($(HAVE_PNGLIB), yes)
	LIBS += -lpng
	SRC += common/im_png.cpp
endif

LIBS += -lm -lz -lX11


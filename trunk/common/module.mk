SRC += common/algebra.cpp common/lc_camera.cpp common/lc_camera_target.cpp common/lc_colors.cpp common/console.cpp common/file.cpp \
       common/lc_flexpiece.cpp common/globals.cpp common/group.cpp common/image.cpp common/im_bmp.cpp common/im_gif.cpp \
       common/lc_application.cpp common/lc_mesh.cpp common/library.cpp common/lc_light.cpp common/mainwnd.cpp \
       common/matrix.cpp common/lc_message.cpp common/minifig.cpp common/lc_model.cpp common/lc_modelref.cpp common/lc_object.cpp common/opengl.cpp \
       common/lc_piece.cpp common/lc_pieceobj.cpp common/pieceinf.cpp common/lc_pivot.cpp common/preview.cpp common/project.cpp common/quant.cpp common/lc_scene.cpp \
       common/str.cpp common/terrain.cpp common/texfont.cpp common/texture.cpp common/view.cpp

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


SRC += common/boundbox.cpp common/camera.cpp common/file.cpp common/globals.cpp common/group.cpp common/image.cpp common/im_bmp.cpp common/im_png.cpp common/library.cpp common/light.cpp common/matrix.cpp common/piece.cpp common/pieceinf.cpp common/project.cpp common/quant.cpp common/terrain.cpp common/texture.cpp common/tr.cpp common/vector.cpp

#LIBS += -lGLU -lGL -ljpeg -lpng -lm -lX11
# Work around for debian bug in mesag-dev
LIBS += -lGLU -lGL -ljpeg -lpng -lz -lm -lX11


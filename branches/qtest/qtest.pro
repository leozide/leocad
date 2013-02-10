#-------------------------------------------------
#
# Project created by QtCreator 2013-02-06T12:31:25
#
#-------------------------------------------------

QT       += core gui opengl

TARGET = qtest
TEMPLATE = app

DEFINES += LC_INSTALL_PREFIX=\\\"/usr/local\\\"
INCLUDEPATH += qt common /usr/include/gtk-2.0 /usr/lib/x86_64-linux-gnu/gtk-2.0/include /usr/include/atk-1.0 /usr/include/cairo /usr/include/gdk-pixbuf-2.0 /usr/include/pango-1.0 /usr/include/gio-unix-2.0/ /usr/include/glib-2.0 /usr/lib/x86_64-linux-gnu/glib-2.0/include /usr/include/pixman-1 /usr/include/freetype2 /usr/include/libpng12
linux {
LIBS += -lX11 -lpng -lgtk-x11-2.0 -lgdk-x11-2.0 -latk-1.0 -lgio-2.0 -lpangoft2-1.0 -lpangocairo-1.0 -lgdk_pixbuf-2.0 -lcairo -lpango-1.0 -lfreetype -lfontconfig -lgobject-2.0 -lglib-2.0   -lpng -lm -lz -lX11 -ldl
}
LIBS += -lz

QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter

SOURCES += \
    common/view.cpp \
    common/tr.cpp \
    common/texfont.cpp \
    common/terrain.cpp \
    common/str.cpp \
    common/quant.cpp \
    common/project.cpp \
    common/preview.cpp \
    common/pieceinf.cpp \
    common/piece.cpp \
    common/opengl.cpp \
    common/object.cpp \
    common/minifig.cpp \
    common/message.cpp \
    common/mainwnd.cpp \
    common/light.cpp \
    common/lc_zipfile.cpp \
    common/lc_texture.cpp \
    common/lc_mesh.cpp \
    common/lc_library.cpp \
    common/lc_file.cpp \
    common/lc_colors.cpp \
    common/lc_application.cpp \
    common/keyboard.cpp \
    common/image.cpp \
    common/im_png.cpp \
    common/im_jpg.cpp \
    common/im_gif.cpp \
    common/im_bmp.cpp \
    common/group.cpp \
    common/globals.cpp \
    common/debug.cpp \
    common/curve.cpp \
    common/console.cpp \
    common/camera.cpp \
    common/array.cpp \
    qt/lc_mainwindow.cpp \
    qt/lc_previewwidget.cpp \
    qt/lc_viewwidget.cpp \
    qt/toolbar.cpp \
    qt/profile.cpp \
    qt/menu.cpp \
    qt/main.cpp \
    qt/linux_gl.cpp \
    qt/libdlg.cpp \
    qt/glwindow.cpp \
    qt/dlgpiece.cpp \
    qt/dlgfile.cpp \
    qt/dialogs.cpp \
    qt/basewnd.cpp \
    qt/system.cpp \
    qt/qtmain.cpp \
    qt/lc_colorlistwidget.cpp

HEADERS  += \
    common/glwindow.h \
    common/array.h \
    common/view.h \
    common/typedefs.h \
    common/tr.h \
    common/texfont.h \
    common/terrain.h \
    common/system.h \
    common/str.h \
    common/quant.h \
    common/project.h \
    common/preview.h \
    common/pieceinf.h \
    common/piece.h \
    common/opengl.h \
    common/object.h \
    common/minifig.h \
    common/message.h \
    common/mainwnd.h \
    common/light.h \
    common/lc_zipfile.h \
    common/lc_texture.h \
    common/lc_mesh.h \
    common/lc_math.h \
    common/lc_library.h \
    common/lc_global.h \
    common/lc_file.h \
    common/lc_colors.h \
    common/lc_application.h \
    common/keyboard.h \
    common/image.h \
    common/group.h \
    common/globals.h \
    common/defines.h \
    common/debug.h \
    common/curve.h \
    common/console.h \
    common/camera.h \
    common/basewnd.h \
    qt/lc_colorlistwidget.h \
    qt/lc_mainwindow.h \
    qt/lc_previewwidget.h \
    qt/lc_viewwidget.h

FORMS    += qt/lc_mainwindow.ui

OTHER_FILES +=

RESOURCES += \
    qt/leocad.qrc

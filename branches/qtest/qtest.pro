QT += core \
    gui \
    opengl
TARGET = qtest
TEMPLATE = app
DEFINES += LC_INSTALL_PREFIX=\\\"/usr/local\\\"
INCLUDEPATH += qt \
    common \
    C:/qt/src/3rdparty/zlib
CONFIG += precompile_header incremental
PRECOMPILED_HEADER = common/lc_global.h
!win32 { 
    LIBS += -lz
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
}
win32:QMAKE_CXXFLAGS_WARN_ON += -wd4100

release: DESTDIR = build/release
debug:   DESTDIR = build/debug

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui

SOURCES += common/view.cpp \
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
    qt/lc_colorlistwidget.cpp \
    qt/lc_glwidget.cpp
HEADERS += common/glwindow.h \
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
    qt/lc_viewwidget.h \
    qt/lc_glwidget.h \
    qt/lc_config.h
FORMS += qt/lc_mainwindow.ui
OTHER_FILES += 
RESOURCES += leocad.qrc

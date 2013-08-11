QT += core gui opengl network
TEMPLATE = app

greaterThan(QT_MAJOR_VERSION, 4) {
	DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0
	QT *= printsupport
}

INCLUDEPATH += qt common
CONFIG += precompile_header incremental
PRECOMPILED_HEADER = common/lc_global.h
win32 { 
	QMAKE_CXXFLAGS_WARN_ON += -wd4100
	DEFINES += _CRT_SECURE_NO_WARNINGS _CRT_SECURE_NO_DEPRECATE=1 _CRT_NONSTDC_NO_WARNINGS=1
	INCLUDEPATH += $$[QT_INSTALL_PREFIX]/src/3rdparty/zlib
	QMAKE_LFLAGS += /INCREMENTAL
	PRECOMPILED_SOURCE = common/lc_global.cpp
	RC_FILE = qt/leocad.rc
	QMAKE_CFLAGS_DEBUG += /O0
	QMAKE_CFLAGS_RELEASE += /Zi
	QMAKE_LFLAGS_RELEASE += /map /debug /opt:ref
} else {
    LIBS += -lz
    QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
}

unix:!macx {
	TARGET = leocad
} else {
	TARGET = LeoCAD
}

CONFIG(debug, debug|release) {
	DESTDIR = build/debug
} else {
	DESTDIR = build/release
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui

unix:!macx {
	isEmpty(INSTALL_PREFIX):INSTALL_PREFIX = /usr
	isEmpty(BIN_DIR):BIN_DIR = $$INSTALL_PREFIX/bin
	isEmpty(DOCS_DIR):DOCS_DIR = $$INSTALL_PREFIX/share/doc/leocad
	isEmpty(ICON_DIR):ICON_DIR = $$INSTALL_PREFIX/share/pixmaps
	isEmpty(MAN_DIR):MAN_DIR = $$INSTALL_PREFIX/share/man/man1
	isEmpty(DESKTOP_DIR):DESKTOP_DIR = $$INSTALL_PREFIX/share/applications
	isEmpty(MIME_DIR):MIME_DIR = $$INSTALL_PREFIX/share/mime/packages
	isEmpty(MIME_ICON_DIR):MIME_ICON_DIR = $$INSTALL_PREFIX/share/icons/hicolor/scalable/mimetypes

	target.path = $$BIN_DIR
	docs.path = $$DOCS_DIR
	docs.files = docs/README.txt docs/CREDITS.txt docs/COPYING.txt
	man.path = $$MAN_DIR
	man.files = leocad.1
	desktop.path = $$DESKTOP_DIR
	desktop.files = qt/leocad.desktop
	icon.path = $$ICON_DIR
	icon.files = leocad.png
	mime.path = $$MIME_DIR
	mime.files = qt/leocad.xml
	mime_icon.path = $$MIME_ICON_DIR
	mime_icon.files = resources/application-vnd.leocad.svg

	INSTALLS += target docs man desktop icon mime mime_icon

	DEFINES += LC_INSTALL_PREFIX=\\\"$$INSTALL_PREFIX\\\"
}

macx {
	ICON = resources/leocad.icns
	QMAKE_INFO_PLIST = qt/Info.plist

	document_icon.files += $$_PRO_FILE_PWD_/resources/leocad_document.icns
	document_icon.path = Contents/Resources
	library.files += $$_PRO_FILE_PWD_/library.bin
	library.path = Contents/Resources

	QMAKE_BUNDLE_DATA += document_icon library
}

SOURCES += common/view.cpp \
    common/tr.cpp \
    common/texfont.cpp \
    common/terrain.cpp \
    common/str.cpp \
    common/project.cpp \
    common/preview.cpp \
    common/pieceinf.cpp \
    common/piece.cpp \
    common/opengl.cpp \
    common/object.cpp \
    common/minifig.cpp \
    common/mainwnd.cpp \
    common/light.cpp \
    common/lc_zipfile.cpp \
    common/lc_texture.cpp \
    common/lc_mesh.cpp \
    common/lc_library.cpp \
    common/lc_file.cpp \
    common/lc_colors.cpp \
    common/lc_application.cpp \
    common/image.cpp \
    common/group.cpp \
    common/debug.cpp \
    common/curve.cpp \
    common/console.cpp \
    common/camera.cpp \
    common/array.cpp \
    common/lc_profile.cpp \
    common/lc_category.cpp \
    qt/lc_qmainwindow.cpp \
    qt/system.cpp \
    qt/qtmain.cpp \
    qt/lc_qpovraydialog.cpp \
    qt/lc_qarraydialog.cpp \
    qt/lc_qgroupdialog.cpp \
    qt/lc_qaboutdialog.cpp \
    qt/lc_qpartstree.cpp \
    qt/lc_qeditgroupsdialog.cpp \
    qt/lc_qselectdialog.cpp \
    qt/lc_qpropertiesdialog.cpp \
    qt/lc_qhtmldialog.cpp \
    qt/lc_qminifigdialog.cpp \
    qt/lc_qpreferencesdialog.cpp \
    qt/lc_qcategorydialog.cpp \
    qt/lc_qprofile.cpp \
    qt/lc_qimagedialog.cpp \
    qt/lc_qapplication.cpp \
    qt/lc_qupdatedialog.cpp \
    qt/lc_qutils.cpp \
    qt/lc_qpropertiestree.cpp \
    qt/lc_qcolorpicker.cpp \
    common/lc_commands.cpp \
    common/lc_shortcuts.cpp \
    qt/lc_qimage.cpp \
    qt/lc_qglwidget.cpp \
    qt/lc_qcolorlist.cpp \
    qt/lc_qfinddialog.cpp
HEADERS += \
    common/array.h \
    common/view.h \
    common/tr.h \
    common/texfont.h \
    common/terrain.h \
    common/system.h \
    common/str.h \
    common/project.h \
    common/preview.h \
    common/pieceinf.h \
    common/piece.h \
    common/opengl.h \
    common/object.h \
    common/minifig.h \
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
    common/image.h \
    common/group.h \
    common/defines.h \
    common/debug.h \
    common/curve.h \
    common/console.h \
    common/camera.h \
    common/basewnd.h \
    common/lc_profile.h \
    common/lc_category.h \
    qt/lc_qmainwindow.h \
    qt/lc_config.h \
    qt/lc_qpovraydialog.h \
    qt/lc_qarraydialog.h \
    qt/lc_qgroupdialog.h \
    qt/lc_qaboutdialog.h \
    qt/lc_qpartstree.h \
    qt/lc_qeditgroupsdialog.h \
    qt/lc_qselectdialog.h \
    qt/lc_qpropertiesdialog.h \
    qt/lc_qhtmldialog.h \
    qt/lc_qminifigdialog.h \
    qt/lc_qpreferencesdialog.h \
    qt/lc_qcategorydialog.h \
    qt/lc_qimagedialog.h \
    qt/lc_qupdatedialog.h \
    qt/lc_qutils.h \
    qt/lc_qpropertiestree.h \
    qt/lc_qcolorpicker.h \
    common/lc_commands.h \
    common/lc_shortcuts.h \
    qt/lc_qglwidget.h \
    qt/lc_qcolorlist.h \
    common/lc_glwidget.h \
    qt/lc_qfinddialog.h
FORMS += \ 
    qt/lc_qpovraydialog.ui \
    qt/lc_qarraydialog.ui \
    qt/lc_qgroupdialog.ui \
    qt/lc_qaboutdialog.ui \
    qt/lc_qeditgroupsdialog.ui \
    qt/lc_qselectdialog.ui \
    qt/lc_qpropertiesdialog.ui \
    qt/lc_qhtmldialog.ui \
    qt/lc_qminifigdialog.ui \
    qt/lc_qpreferencesdialog.ui \
    qt/lc_qcategorydialog.ui \
    qt/lc_qimagedialog.ui \
    qt/lc_qupdatedialog.ui \
    qt/lc_qfinddialog.ui
OTHER_FILES += 
RESOURCES += leocad.qrc

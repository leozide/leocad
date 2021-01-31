QT += core gui opengl network xml concurrent
QT *= printsupport
TEMPLATE = app

lessThan(QT_MAJOR_VERSION, 5) {
	error("LeoCAD requires Qt5.4 or later.")
}

equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 4) {
	error("LeoCAD requires Qt5.4 or later.")
}

qtHaveModule(gamepad) {
	QT += gamepad
	DEFINES += LC_ENABLE_GAMEPAD
}

INCLUDEPATH += qt common
CONFIG += precompile_header incremental c++14 force_debug_info

win32 {
	RC_ICONS = resources/leocad.ico
	VERSION = 21.1.0.0
	QMAKE_TARGET_COMPANY = LeoCAD Software
	QMAKE_TARGET_DESCRIPTION = LeoCAD
	QMAKE_TARGET_COPYRIGHT = "Copyright (C) LeoCAD.org"
	QMAKE_TARGET_PRODUCT = LeoCAD
	RC_CODEPAGE = 1252
	RC_LANG = 0x0409

	LIBS += -lwininet
}
win32-msvc* {
	QMAKE_CXXFLAGS_WARN_ON += /W4 -wd4100
	QMAKE_CXXFLAGS *= /MP
	PRECOMPILED_HEADER = common/lc_global.h
	PRECOMPILED_SOURCE = common/lc_global.cpp
	DEFINES += _CRT_SECURE_NO_WARNINGS _CRT_SECURE_NO_DEPRECATE=1 _CRT_NONSTDC_NO_WARNINGS=1
	INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtZlib
	QMAKE_LFLAGS += /INCREMENTAL
	LIBS += -ladvapi32 -lshell32 -lopengl32 -luser32
} else {
	PRECOMPILED_HEADER = common/lc_global.h
	LIBS += -lz
	QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
}

isEmpty(QMAKE_LRELEASE) {
	win32:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]\\lrelease.exe
	else:QMAKE_LRELEASE = $$[QT_INSTALL_BINS]/lrelease
	unix {
		!exists($$QMAKE_LRELEASE) { QMAKE_LRELEASE = lrelease-qt4 }
	} else {
		!exists($$QMAKE_LRELEASE) { QMAKE_LRELEASE = lrelease }
	}
}

TRAVIS_TAG = $$(TRAVIS_TAG)
TRAVIS_COMMIT = $$(TRAVIS_COMMIT)
!isEmpty(TRAVIS_COMMIT) {
	isEmpty(TRAVIS_TAG) {
		DEFINES += "LC_CONTINUOUS_BUILD=$$system(git rev-parse --short HEAD)"
	}
}

APPVEYOR_REPO_TAG = $$(APPVEYOR_REPO_TAG)
APPVEYOR_REPO_COMMIT = $$(APPVEYOR_REPO_COMMIT)
!isEmpty(APPVEYOR_REPO_COMMIT) {
	isEmpty(APPVEYOR_REPO_TAG) {
		DEFINES += "LC_CONTINUOUS_BUILD=$$system(git rev-parse --short HEAD)"
		VERSION = 0.0.0.1
		QMAKE_TARGET_DESCRIPTION = "LeoCAD Continuous Build $$system(git rev-parse --short HEAD)"
	}
}

TSFILES = resources/leocad_fr.ts resources/leocad_pt.ts resources/leocad_de.ts resources/leocad_uk.ts resources/leocad_cs.ts resources/leocad_es.ts
lrelease.input = TSFILES
lrelease.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
lrelease.commands = $$QMAKE_LRELEASE -silent ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
lrelease.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += lrelease

system($$QMAKE_LRELEASE -silent $$TSFILES)

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
	isEmpty(ICON_DIR):ICON_DIR = $$INSTALL_PREFIX/share/icons
	isEmpty(MAN_DIR):MAN_DIR = $$INSTALL_PREFIX/share/man/man1
	isEmpty(DESKTOP_DIR):DESKTOP_DIR = $$INSTALL_PREFIX/share/applications
	isEmpty(MIME_DIR):MIME_DIR = $$INSTALL_PREFIX/share/mime/packages
	isEmpty(APPDATA_DIR):APPDATA_DIR = $$INSTALL_PREFIX/share/metainfo

	target.path = $$BIN_DIR
	docs.path = $$DOCS_DIR
	docs.files = docs/README.txt docs/CREDITS.txt docs/COPYING.txt
	man.path = $$MAN_DIR
	man.files = docs/leocad.1
	desktop.path = $$DESKTOP_DIR
	desktop.files = qt/leocad.desktop
	icons.path = $$ICON_DIR/hicolor
	icons.files += tools/icon/128x128
	icons.files += tools/icon/16x16
	icons.files += tools/icon/24x24
	icons.files += tools/icon/256x256
	icons.files += tools/icon/32x32
	icons.files += tools/icon/48x48
	icons.files += tools/icon/512x512
	icons.files += tools/icon/64x64
	icons.files += tools/icon/scalable
	mime.path = $$MIME_DIR
	mime.files = qt/leocad.xml
	appdata.path = $$APPDATA_DIR
	appdata.files = tools/setup/leocad.appdata.xml

	INSTALLS += target docs man desktop icons mime appdata

	!isEmpty(DISABLE_UPDATE_CHECK) {
		DEFINES += LC_DISABLE_UPDATE_CHECK=$$DISABLE_UPDATE_CHECK
	}

	!isEmpty(LDRAW_LIBRARY_PATH) {
		DEFINES += LC_LDRAW_LIBRARY_PATH=\\\"$$LDRAW_LIBRARY_PATH\\\"
	}
}

macx {
	ICON = resources/leocad.icns
	QMAKE_INFO_PLIST = qt/Info.plist

	document_icon.files += $$_PRO_FILE_PWD_/resources/leocad_document.icns
	document_icon.path = Contents/Resources
	library.files += $$_PRO_FILE_PWD_/library.bin
	library.path = Contents/Resources
	povray.files += $$_PRO_FILE_PWD_/povray
	povray.path = Contents/MacOS

	QMAKE_BUNDLE_DATA += document_icon library povray
	DEFINES += LC_DISABLE_UPDATE_CHECK=1
}

SOURCES += \
	common/texfont.cpp \
	common/project.cpp \
	common/pieceinf.cpp \
	common/piece.cpp \
	common/object.cpp \
	common/minifig.cpp \
	common/light.cpp \
	common/lc_application.cpp \
	common/lc_category.cpp \
	common/lc_collapsiblewidget.cpp \
	common/lc_colors.cpp \
	common/lc_commands.cpp \
	common/lc_context.cpp \
	common/lc_edgecolordialog.cpp \
	common/lc_file.cpp \
	common/lc_findreplacewidget.cpp \
	common/lc_glextensions.cpp \
	common/lc_http.cpp \
	common/lc_instructions.cpp \
	common/lc_instructionsdialog.cpp \
	common/lc_library.cpp \
	common/lc_lxf.cpp \
	common/lc_mainwindow.cpp \
	common/lc_mesh.cpp \
	common/lc_meshloader.cpp \
	common/lc_minifigdialog.cpp \
	common/lc_model.cpp \
	common/lc_pagesetupdialog.cpp \
	common/lc_partselectionwidget.cpp \
	common/lc_previewwidget.cpp \
	common/lc_profile.cpp \
	common/lc_scene.cpp \
	common/lc_shortcuts.cpp \
	common/lc_stringcache.cpp \
	common/lc_synth.cpp \
	common/lc_texture.cpp \
	common/lc_timelinewidget.cpp \
	common/lc_view.cpp \
	common/lc_viewsphere.cpp \
	common/lc_viewwidget.cpp \
	common/lc_zipfile.cpp \
	common/image.cpp \
	common/group.cpp \
	common/camera.cpp \
	qt/system.cpp \
	qt/qtmain.cpp \
	qt/lc_qarraydialog.cpp \
	qt/lc_qgroupdialog.cpp \
	qt/lc_qaboutdialog.cpp \
	qt/lc_qeditgroupsdialog.cpp \
	qt/lc_qselectdialog.cpp \
	qt/lc_qpropertiesdialog.cpp \
	qt/lc_qhtmldialog.cpp \
	qt/lc_qpreferencesdialog.cpp \
	qt/lc_qcategorydialog.cpp \
	qt/lc_qimagedialog.cpp \
	qt/lc_qupdatedialog.cpp \
	qt/lc_qutils.cpp \
	qt/lc_qpropertiestree.cpp \
	qt/lc_qcolorpicker.cpp \
	qt/lc_qcolorlist.cpp \
	qt/lc_qmodellistdialog.cpp \
	qt/lc_renderdialog.cpp \
	qt/lc_setsdatabasedialog.cpp \
	common/lc_partpalettedialog.cpp
HEADERS += \
	common/texfont.h \
	common/project.h \
	common/pieceinf.h \
	common/piece.h \
	common/object.h \
	common/minifig.h \
	common/light.h \
	common/lc_application.h \
	common/lc_array.h \
	common/lc_category.h \
	common/lc_collapsiblewidget.h \
	common/lc_colors.h \
	common/lc_commands.h \
	common/lc_context.h \
	common/lc_edgecolordialog.h \
	common/lc_file.h \
	common/lc_findreplacewidget.h \
	common/lc_glextensions.h \
	common/lc_global.h \
	common/lc_http.h \
	common/lc_instructions.h \
	common/lc_instructionsdialog.h \
	common/lc_library.h \
	common/lc_lxf.h \
	common/lc_mainwindow.h \
	common/lc_math.h \
	common/lc_mesh.h \
	common/lc_meshloader.h \
	common/lc_minifigdialog.h \
	common/lc_model.h \
	common/lc_pagesetupdialog.h \
	common/lc_previewwidget.h \
	common/lc_profile.h \
	common/lc_scene.h \
	common/lc_shortcuts.h \
	common/lc_stringcache.h \
	common/lc_synth.h \
	common/lc_texture.h \
	common/lc_view.h \
	common/lc_viewsphere.h \
	common/lc_viewwidget.h \
	common/lc_zipfile.h \
	common/lc_partselectionwidget.h \
	common/lc_timelinewidget.h \
	common/image.h \
	common/group.h \
	common/camera.h \
	qt/lc_qarraydialog.h \
	qt/lc_qgroupdialog.h \
	qt/lc_qaboutdialog.h \
	qt/lc_qeditgroupsdialog.h \
	qt/lc_qselectdialog.h \
	qt/lc_qpropertiesdialog.h \
	qt/lc_qhtmldialog.h \
	qt/lc_qpreferencesdialog.h \
	qt/lc_qcategorydialog.h \
	qt/lc_qimagedialog.h \
	qt/lc_qupdatedialog.h \
	qt/lc_qutils.h \
	qt/lc_qpropertiestree.h \
	qt/lc_qcolorpicker.h \
	qt/lc_qcolorlist.h \
	qt/lc_qmodellistdialog.h \
	qt/lc_renderdialog.h \
	qt/lc_setsdatabasedialog.h \
	common/lc_partpalettedialog.h
FORMS += \
	qt/lc_qarraydialog.ui \
	qt/lc_qgroupdialog.ui \
	qt/lc_qaboutdialog.ui \
	qt/lc_qeditgroupsdialog.ui \
	qt/lc_qselectdialog.ui \
	qt/lc_qpropertiesdialog.ui \
	qt/lc_qhtmldialog.ui \
	qt/lc_qpreferencesdialog.ui \
	qt/lc_qcategorydialog.ui \
	qt/lc_qimagedialog.ui \
	qt/lc_qupdatedialog.ui \
	qt/lc_qmodellistdialog.ui \
	qt/lc_renderdialog.ui \
	qt/lc_setsdatabasedialog.ui \
	common/lc_minifigdialog.ui \
	common/lc_pagesetupdialog.ui \
	common/lc_partpalettedialog.ui
OTHER_FILES +=
RESOURCES += leocad.qrc resources/stylesheet/stylesheet.qrc

!win32 {
	TRANSLATIONS = resources/leocad_pt.ts resources/leocad_fr.ts resources/leocad_de.ts resources/leocad_uk.ts resources/leocad_cs.ts resources/leocad_es.ts
}

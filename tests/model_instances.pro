# Reuse the application sources and resources, replacing only its entry point.
include(../leocad.pro)

MODEL_INSTANCE_ROOT = $$clean_path($$PWD/..)

SOURCES -= qt/qtmain.cpp

for(source, SOURCES): MODEL_INSTANCE_SOURCES += $$absolute_path($$source, $$MODEL_INSTANCE_ROOT)
for(header, HEADERS): MODEL_INSTANCE_HEADERS += $$absolute_path($$header, $$MODEL_INSTANCE_ROOT)
for(form, FORMS): MODEL_INSTANCE_FORMS += $$absolute_path($$form, $$MODEL_INSTANCE_ROOT)
for(resource, RESOURCES): MODEL_INSTANCE_RESOURCES += $$absolute_path($$resource, $$MODEL_INSTANCE_ROOT)
for(directory, INCLUDEPATH): MODEL_INSTANCE_INCLUDES += $$absolute_path($$directory, $$MODEL_INSTANCE_ROOT)

SOURCES = $$MODEL_INSTANCE_SOURCES $$PWD/lc_modelinstance_test.cpp
HEADERS = $$MODEL_INSTANCE_HEADERS
FORMS = $$MODEL_INSTANCE_FORMS
RESOURCES = $$MODEL_INSTANCE_RESOURCES
INCLUDEPATH = $$MODEL_INSTANCE_INCLUDES
PRECOMPILED_HEADER = $$MODEL_INSTANCE_ROOT/common/lc_global.h
win32-msvc*: PRECOMPILED_SOURCE = $$MODEL_INSTANCE_ROOT/common/lc_global.cpp

QT += testlib
CONFIG += console testcase
CONFIG -= app_bundle
TARGET = leocad_modelinstance_tests
QMAKE_INFO_PLIST = $$MODEL_INSTANCE_ROOT/qt/Info.plist
RC_ICONS =
QMAKE_BUNDLE_DATA =
INSTALLS =
QMAKE_EXTRA_COMPILERS =

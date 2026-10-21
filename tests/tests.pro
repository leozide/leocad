TEMPLATE = app
TARGET = leocad_tests

CONFIG += console testcase c++17
CONFIG -= app_bundle

QT += concurrent opengl printsupport testlib widgets

greaterThan(QT_MAJOR_VERSION, 5) {
	QT += openglwidgets
}

INCLUDEPATH += ../common

HEADERS += lc_tests.h

SOURCES += \
	lc_objecthistory_test.cpp \
	main.cpp

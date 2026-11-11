TEMPLATE = app
TARGET = leocad_tests

CONFIG += console testcase
equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 12) {
	CONFIG += c++1z
	win32-msvc*: QMAKE_CXXFLAGS += /std:c++17
	else: QMAKE_CXXFLAGS += -std=c++17
} else {
	CONFIG += c++17
}
CONFIG -= app_bundle

QT += concurrent opengl printsupport testlib widgets

greaterThan(QT_MAJOR_VERSION, 5) {
	QT += openglwidgets
}

INCLUDEPATH += ../common

HEADERS += \
	../common/lc_connectivity.h \
	../common/lc_shadowparser.h \
	lc_tests.h

SOURCES += \
	../common/lc_connectivity.cpp \
	../common/lc_math.cpp \
	../common/lc_objectproperty.cpp \
	../common/lc_shadowparser.cpp \
	lc_connectivity_test.cpp \
	lc_math_test.cpp \
	lc_objecthistory_test.cpp \
	lc_objectproperty_test.cpp \
	main.cpp

QT += network websockets core sql
QT -= gui

TEMPLATE = app

unix: CONFIG += shared static c++1z
win32: CONFIG+= shared static c++17

coverage {
    QMAKE_CXXFLAGS += --coverage -g -Og    # -fprofile-arcs -ftest-coverage
    LIBS += -lgcov
    CONFIG -= static
}

# Needed so that Windows doesn't do `release/` and `debug/` subfolders
# in the output directory.
CONFIG -= \
        copy_dir_files \
        debug_and_release \
        debug_and_release_target

DESTDIR = $$PWD/../bin

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Enable this to print network messages to the console
DEFINES += NET_DEBUG

INCLUDEPATH += $$PWD/src

SOURCES += $$files($$PWD/src/*.cpp, true)

HEADERS += $$files($$PWD/src/*.h, true)

CONFIG+=DEBUG

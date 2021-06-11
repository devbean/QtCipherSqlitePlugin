QT += testlib sql
TEMPLATE = app
TARGET = sqlitecipher_test_plugin

CONFIG  += c++11

unix: CONFIG += testcase
win32: {
    CONFIG += build_all
    QMAKE_SUBSTITUTES += qt_conf
    qt_conf.input = qt.conf.in
    build_pass: CONFIG(debug, debug|release) {
        qt_conf.output = debug/qt.conf
    }
    else: build_pass {
        qt_conf.output = release/qt.conf
    }
} else {
    QMAKE_SUBSTITUTES += qt.conf.in
}
ios: {
    CONFIG(debug, debug|release) {
        LIBS += -lsqlitecipher_debug
    } else {
        LIBS += -lsqlitecipher
    }
}

# The following define makes your compiler warn you if you use any
# feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Input
SOURCES += main.cpp

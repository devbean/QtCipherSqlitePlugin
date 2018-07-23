QT       += core sql

QT       -= gui

TARGET    = test
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

ios {
    CONFIG(debug, debug|release) {
        LIBS += -lsqlitecipher_debug
    } else {
        LIBS += -lsqlitecipher
    }
}

SOURCES += main.cpp
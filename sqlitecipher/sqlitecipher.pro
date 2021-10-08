TARGET = sqlitecipher

android {
    TEMPLATE = app
} else {
    TEMPLATE = lib
}

QT_FOR_CONFIG += sqldrivers-private

CONFIG  += c++11 plugin

include($$PWD/sqlite3/sqlite3.pri)

target.path = $$[QT_INSTALL_PLUGINS]/sqldrivers/
INSTALLS += target

HEADERS  += \
    $$PWD/sqlitecipher_p.h \
    $$PWD/sqlitecipher_global.h
SOURCES  += \
    $$PWD/smain.cpp \
    $$PWD/sqlitecipher.cpp
OTHER_FILES += SqliteCipherDriverPlugin.json

!system-sqlite:!contains( LIBS, .*sqlite.* ) {
    CONFIG(release, debug|release):DEFINES *= NDEBUG
    DEFINES += SQLITE_OMIT_LOAD_EXTENSION SQLITE_OMIT_COMPLETE SQLITE_ENABLE_FTS3 SQLITE_ENABLE_FTS3_PARENTHESIS SQLITE_ENABLE_RTREE SQLITE_USER_AUTHENTICATION
    !contains(CONFIG, largefile):DEFINES += SQLITE_DISABLE_LFS
    winrt: DEFINES += SQLITE_OS_WINRT
    winphone: DEFINES += SQLITE_WIN32_FILEMAPPING_API=1
    qnx: DEFINES += _QNX_SOURCE
} else {
    LIBS += $$QT_LFLAGS_SQLITE
    QMAKE_CXXFLAGS *= $$QT_CFLAGS_SQLITE
}

QT = core core-private sql-private

PLUGIN_CLASS_NAME = SqliteCipherDriverPlugin

PLUGIN_TYPE = sqldrivers
load(qt_plugin)

DEFINES += QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_ASCII

QMAKE_CFLAGS += -march=native

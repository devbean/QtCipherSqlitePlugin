TARGET = sqlitecipher
android {
    TEMPLATE = app
} else {
    TEMPLATE = lib
}

QT      *= core sql

include($$PWD/qt_p.pri)
include($$PWD/sqlite3/sqlite3.pri)

target.path = $$[QT_INSTALL_PLUGINS]/sqldrivers/
INSTALLS += target

HEADERS  += $$PWD/qsql_sqlite.h
SOURCES  += $$PWD/qsql_sqlite.cpp \
    $$PWD/smain.cpp
OTHER_FILES += SqliteCipherDriverPlugin.json

!system-sqlite:!contains( LIBS, .*sqlite.* ) {
    CONFIG(release, debug|release):DEFINES *= NDEBUG
    DEFINES     += SQLITE_OMIT_LOAD_EXTENSION SQLITE_OMIT_COMPLETE QT_PLUGIN
    blackberry: DEFINES += SQLITE_ENABLE_FTS3 SQLITE_ENABLE_FTS3_PARENTHESIS SQLITE_ENABLE_RTREE
    wince*: DEFINES += HAVE_LOCALTIME_S=0
} else {
    LIBS *= $$QT_LFLAGS_SQLITE
    QMAKE_CXXFLAGS *= $$QT_CFLAGS_SQLITE
}

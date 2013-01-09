TARGET = sqlitecipher
 
HEADERS  = qsql_sqlite.h sqlite3.h
SOURCES  = smain.cpp \
           qsql_sqlite.cpp

!system-sqlite:!contains( LIBS, .*sqlite.* ) {
    CONFIG(release, debug|release):DEFINES *= NDEBUG
    DEFINES     += SQLITE_OMIT_LOAD_EXTENSION SQLITE_OMIT_COMPLETE SQLITE_HAS_CODEC
    INCLUDEPATH += include
    LIBS        += ./lib/libsqlite.a
} else {
    LIBS *= $$QT_LFLAGS_SQLITE
    QMAKE_CXXFLAGS *= $$QT_CFLAGS_SQLITE
}
 
include(../qsqldriverbase.pri)

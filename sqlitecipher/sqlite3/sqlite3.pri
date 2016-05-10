CONFIG(release, debug|release):DEFINES *= NDEBUG

DEFINES += SQLITE_HAS_CODEC CODEC_TYPE=CODEC_TYPE_AES256 SQLITE_CORE SQLITE_SECURE_DELETE SQLITE_ENABLE_COLUMN_METADATA SQLITE_ENABLE_RTREE USE_DYNAMIC_SQLITE3_LOAD=0

INCLUDEPATH += $$PWD
DEPENDPATH  += $$PWD

HEADERS += \
    $$PWD/sqlite3.h \
    $$PWD/codec.h \
    $$PWD/rijndael.h \
    $$PWD/sha2.h \
    $$PWD/sqlite3ext.h \
    $$PWD/sqlite3userauth.h

SOURCES += \
    $$PWD/sqlite3.c \
    $$PWD/sqlite3secure.c \
    $$PWD/codec.c \
    $$PWD/rijndael.c \
    $$PWD/codecext.c \
    $$PWD/extensionfunctions.c \
    $$PWD/sha2.c \
    $$PWD/userauth.c

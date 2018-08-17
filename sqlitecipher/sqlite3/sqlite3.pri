CONFIG(release, debug|release):DEFINES *= NDEBUG

DEFINES += _CRT_SECURE_NO_WARNINGS _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE THREADSAFE=1 SQLITE_MAX_ATTACHED=10 SQLITE_SOUNDEX SQLITE_ENABLE_EXPLAIN_COMMENTS SQLITE_ENABLE_COLUMN_METADATA SQLITE_HAS_CODEC=1 CODEC_TYPE=CODEC_TYPE_CHACHA20 SQLITE_SECURE_DELETE SQLITE_ENABLE_FTS3 SQLITE_ENABLE_FTS3_PARENTHESIS SQLITE_ENABLE_FTS4 SQLITE_ENABLE_FTS5 SQLITE_ENABLE_JSON1 SQLITE_ENABLE_RTREE SQLITE_CORE SQLITE_ENABLE_EXTFUNC SQLITE_ENABLE_CSV SQLITE_ENABLE_SHA3 SQLITE_ENABLE_CARRAY SQLITE_ENABLE_FILEIO SQLITE_ENABLE_SERIES SQLITE_TEMP_STORE=2 SQLITE_USE_URI SQLITE_USER_AUTHENTICATION

win32-msvc* {
    # Nothing for now.
}
android-g++ {
    DEFINES += restrict=__restrict
}
win32-g++ {
    DEFINES += restrict=__restrict
}

INCLUDEPATH += $$PWD
DEPENDPATH  += $$PWD

HEADERS += \
    $$PWD/codec.h \
    $$PWD/fastpbkdf2.h \
    $$PWD/rijndael.h \
    $$PWD/sha1.h \
    $$PWD/sha2.h \
    $$PWD/sqlite3.h \
    $$PWD/sqlite3ext.h \
    $$PWD/sqlite3secure.h \
    $$PWD/sqlite3userauth.h \
    $$PWD/test_windirent.h

SOURCES += \
    $$PWD/carray.c \
    $$PWD/chacha20poly1305.c \
    $$PWD/codec.c \
    $$PWD/codecext.c \
    $$PWD/csv.c \
    $$PWD/extensionfunctions.c \
    $$PWD/fastpbkdf2.c \
    $$PWD/fileio.c \
    $$PWD/md5.c \
    $$PWD/regexp.c \
    $$PWD/rekeyvacuum.c \
    $$PWD/rijndael.c \
    $$PWD/series.c \
    $$PWD/sha1.c \
    $$PWD/sha2.c \
    $$PWD/shathree.c \
    $$PWD/sqlite3.c \
    $$PWD/sqlite3secure.c \
    $$PWD/test_windirent.c \
    $$PWD/userauth.c

OTHER_FILES += \
    $$PWD/sqlite3.def \
    $$PWD/sqlite3.rc \
    $$PWD/sqlite3shell.rc

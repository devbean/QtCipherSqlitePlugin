# You should set INCLUDEPATH according to your Qt code tree.
# Note that the end should be sql/kernel or some other directory
# which contains the file qsqlcachedresult_p.h

# Qt4
!greaterThan(QT_MAJOR_VERSION, 4) {
    QT4_SRC_PATH=
    isEmpty(QT4_SRC_PATH) {
        error(Set QT4_SRC_PATH in qt_p.pri first)
    }
    INCLUDEPATH += $$QT4_SRC_PATH/src/sql/kernel
}

# Qt5
greaterThan(QT_MAJOR_VERSION, 4) {
    QT5_SRC_PATH=D:\Develop\Qt\Qt5.5.1\5.5\Src\qtbase\src
    isEmpty(QT5_SRC_PATH) {
        error(Set QT5_SRC_PATH in qt_p.pri first)
    }
    INCLUDEPATH += $$QT5_SRC_PATH\sql\kernel
}

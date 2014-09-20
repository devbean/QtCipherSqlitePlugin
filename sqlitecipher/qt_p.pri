# You should set INCLUDEPATH according to your Qt code tree.
# Note that the end should be sql/kernel or some other directory
# which contains the file qsqlcachedresult_p.h

# Qt4
!greaterThan(QT_MAJOR_VERSION, 4): INCLUDEPATH += <Qt4_Path>/src/sql/kernel

# Qt5
greaterThan(QT_MAJOR_VERSION, 4): INCLUDEPATH += <Qt5_Path>/Src/qtbase/src/sql/kernel

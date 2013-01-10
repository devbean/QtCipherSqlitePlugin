# You should set INCLUDEPATH according to your Qt code tree.
# Note that the end should be sql/kernel or some other directory
# which contains the file qsqlcachedresult_p.h

# Qt4
!greaterThan(QT_MAJOR_VERSION, 4): INCLUDEPATH += D:/Develop/Qt/4.8.4/src/sql/kernel

# Qt5
greaterThan(QT_MAJOR_VERSION, 4): INCLUDEPATH += D:/Develop/Qt/5.0.0/5.0.0/Src/qtbase/src/sql/kernel

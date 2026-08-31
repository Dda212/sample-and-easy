QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    appointment.cpp \
    clinic.cpp \
    main.cpp \
    mainwindow.cpp \
    addclinicdialog.cpp \
    bookdialog.cpp

HEADERS += \
    appointment.h \
    clinic.h \
    mainwindow.h \
    addclinicdialog.h \
    bookdialog.h

FORMS += \
    mainwindow.ui \
    addclinicdialog.ui \
    bookdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

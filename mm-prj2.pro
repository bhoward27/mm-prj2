QT       += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    lzw.cpp \
    main.cpp \
    mainwindow.cpp \
    pngwindow.cpp \
    utils.cpp \
    wav.cpp \
    wav_compression.cpp \
    wavwindow.cpp

HEADERS += \
    lzw.h \
    mainwindow.h \
    pngwindow.h \
    utils.h \
    wav.h \
    wav_compression.h \
    wavwindow.h

FORMS += \
    mainwindow.ui \
    pngwindow.ui \
    wavwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    Log/log_file.cpp \
    Log/log_printf.cpp \
    RegistryHelper.cpp \
    main.cpp \
    mainwindow.cpp \
    processer.cpp \
    resample.cpp

HEADERS += \
    Log/LogHelper.h \
    Log/log_func.h \
    Log/log_macro.h \
    RegistryHelper.h \
    arch.h \
    mainwindow.h \
    processer.h \
    resample_neon.h \
    resample_sse.h \
    speex_resampler.h

FORMS += \
    mainwindow.ui

LIBS += -lAdvAPI32
QT += axcontainer

DEFINES += OUTSIDE_SPEEX \
           FLOATING_POINT \
           USE_SSE

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

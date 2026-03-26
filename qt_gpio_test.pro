QT += widgets
CONFIG += c++17

SOURCES += \
    Gpio.c \
    main.cpp \
    mainwindow.cpp \
    gpio_interface.cpp \
    Motor.c \
    Proc_fw.c \
    timer.c

HEADERS += \
    Gpio.h \
    mainwindow.h \
    gpio_interface.h \
    Motor.h \
    Proc_fw.h \
    timer.h


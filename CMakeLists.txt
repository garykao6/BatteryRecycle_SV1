cmake_minimum_required(VERSION 3.19)
project(BatteryRecycle_SV1 LANGUAGES CXX C)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 交叉編譯 sysroot
#set(CMAKE_SYSROOT $ENV{HOME}/rpi-sysroot)

# RPATH (Pi 上 Qt 安裝路徑，視實際情況調整)
set(CMAKE_INSTALL_RPATH "/usr/local/qt6/lib")
set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Widgets Multimedia MultimediaWidgets Mqtt SerialPort )
find_package(OpenGL REQUIRED COMPONENTS GLES3)

qt_standard_project_setup()

# --- Source / Header / UI / Resource files ---
set(SOURCES
    main.cpp
    mainwindow.cpp
    gpio_interface.cpp
    playtool.cpp
    homepage.cpp
    mb_endpage.cpp
    mb_loginpage.cpp
    mb_putinbatterypage.cpp
    clockbus.cpp
    global.cpp
    DeviceAPI.c
    Gpio.c
    Motor.c
    Proc_fw.c
    timer.c
    ws281x.c
    devicemonitor.cpp
    log.c
)

set(HEADERS
    mainwindow.h
    gpio_interface.h
    playtool.h
    homepage.h
    mb_endpage.h
    mb_loginpage.h
    mb_putinbatterypage.h
    clockbus.h
    global.h

    DeviceAPI.h
    Gpio.h
    Motor.h
    Proc_fw.h
    timer.h
    ws281x.h
    devicemonitor.h
    log.h
)

set(UIS
    homepage.ui
    mb_endpage.ui
    mb_loginpage.ui
    mb_putinbatterypage.ui
    playtool.ui
    erroritem_dialog.ui
    loginerr_dialog.ui
    machineerr_dialog.ui
    machinerecord_dialog.ui
    nmb_endpage.ui
    nmb_putinbatterypage.ui
    pointupload_dialog.ui
    service_dialog.ui
    welcom_dialog.ui
)


set(RESOURCES
    resource.qrc
)

# --- Executable target ---
qt_add_executable(BatteryRecycle_SV1
    ${SOURCES}
    ${HEADERS}
    ${UIS}
    ${RESOURCES}
    mqtthelper.h mqtthelper.cpp
    erroritem_dialog.ui homepage.ui loginerr_dialog.ui machineerr_dialog.ui machinerecord_dialog.ui mb_endpage.ui mb_loginpage.ui mb_putinbatterypage.ui nmb_endpage.ui nmb_putinbatterypage.ui playtool.ui pointupload_dialog.ui service_dialog.ui welcom_dialog.ui
    clockbus.h DeviceAPI.h erroritem_dialog.h  global.h Gpio.h gpiod.h gpio_interface.h homepage.h idlewatcher.h loginerr_dialog.h machineerr_dialog.h machinerecord_dialog.h mainwindow.h mb_endpage.h mb_loginpage.h mb_putinbatterypage.h moc_predefs.h Motor.h mqtthelper.h nmb_endpage.h nmb_putinbatterypage.h playtool.h pointupload_dialog.h Proc_fw.h service_dialog.h timer.h               welcom_dialog.h ws281x.h
    clockbus.cpp DeviceAPI.c erroritem_dialog.cpp global.cpp Gpio.c gpio_interface.cpp homepage.cpp idlewatcher.cpp loginerr_dialog.cpp machineerr_dialog.cpp machinerecord_dialog.cpp main.cpp mainwindow.cpp mb_endpage.cpp mb_loginpage.cpp mb_putinbatterypage.cpp Motor.c mqtthelper.cpp nmb_endpage.cpp nmb_putinbatterypage.cpp playtool.cpp pointupload_dialog.cpp Proc_fw.c service_dialog.cpp timer.c welcom_dialog.cpp ws281x.c
    glable.h
    backstagelogin_page.h backstagelogin_page.cpp backstagelogin_page.ui
    backstage.h backstage.cpp backstage.ui
    setting_dialog.h setting_dialog.cpp setting_dialog.ui

)

# --- 找到 gpiod 和 ws2811 庫 ---
find_library(GPIOD_LIB gpiod HINTS ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu)
find_library(WS2811_LIB ws2811 HINTS ${CMAKE_SYSROOT}/usr/local/lib)

if(NOT GPIOD_LIB)
    message(FATAL_ERROR "Cannot find libgpiod in sysroot")
endif()

if(NOT WS2811_LIB)
    message(FATAL_ERROR "Cannot find libws2811 in sysroot")
endif()

# --- Libraries ---
link_directories(
    ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu
    ${CMAKE_SYSROOT}/usr/local/lib
)

target_link_libraries(BatteryRecycle_SV1
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Multimedia
        Qt::MultimediaWidgets
        Qt6::Mqtt
        Qt6::SerialPort
        OpenGL::GLES3
        ${GPIOD_LIB}     # ✅ libgpiod
        ${WS2811_LIB}    # ✅ libws2811
        pigpio
        pthread
        rt
        Qt6::Network
)

# --- Include directories ---
target_include_directories(BatteryRecycle_SV1
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SYSROOT}/usr/include
        ${CMAKE_SYSROOT}/usr/include/aarch64-linux-gnu
        ${CMAKE_SYSROOT}/usr/local/include  # ✅ libws2811 頭文件
        ${CMAKE_SYSROOT}/usr/local/include/ws2811  # 如果 ws2811.h 在子目錄
)

# ✅ 若為交叉編譯，加入 rpath-link（針對 pulseaudio）
target_link_options(BatteryRecycle_SV1 PRIVATE
    -Wl,-rpath-link,/home/kao/workspace/qt-rpi-cross-compilation/rpi-sysroot/usr/lib/aarch64-linux-gnu/pulseaudio
)

# --- Install rules ---
include(GNUInstallDirs)
install(TARGETS BatteryRecycle_SV1
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET BatteryRecycle_SV1
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})

message(STATUS "CMAKE_SYSROOT = ${CMAKE_SYSROOT}")

#include "gpio_interface.h"
#include <pigpio.h>
#include <iostream>
#include "Motor.h"

GPIOInterface::GPIOInterface(int pin) : gpioPin(pin) {
    if (gpioInitialise() < 0) {
        std::cerr << "pigpio 初始化失敗" << std::endl;
    } else {
        gpioSetMode(gpioPin, PI_OUTPUT);
    }
}

GPIOInterface::~GPIOInterface() {
    gpioWrite(gpioPin, 1);
    gpioTerminate();
}

void GPIOInterface::setState(bool on) {
    gpioWrite(gpioPin, on ? 0 : 1);
}

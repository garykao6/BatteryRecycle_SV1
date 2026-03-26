#ifndef GPIO_INTERFACE_H
#define GPIO_INTERFACE_H

class GPIOInterface {
public:
    GPIOInterface(int pin);
    ~GPIOInterface();

    void setState(bool on);

private:
    int gpioPin;
};

#endif

#include "Motor.h"
#include "Gpio.h"
#include <pigpio.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <string.h>

//static int pwm_pin = 20;
//static int pwm_freq = PWM_SPEED;  // 4kHz
static int spi_fd = -1;

//////////////////////////////////////////////////////
// SPI ?��???
//////////////////////////////////////////////////////
int spi_init(const char *device, uint8_t mode, uint32_t speed)
{
    spi_fd = open(device, O_RDWR);
    if (spi_fd < 0) {
        perror("SPI open failed");
        return -1;
    }

    if (ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) == -1) {
        perror("Can't set SPI mode");
        return -1;
    }

    if (ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
        perror("Can't set SPI speed");
        return -1;
    }

    return 0;
}

//////////////////////////////////////////////////////
// SPI 寫�?存器
//////////////////////////////////////////////////////
void TMC2240_WriteReg(uint8_t address, uint32_t value)
{
    uint8_t tx[5];
    tx[0] = address | 0x80; // bit7=1 表示�?
    tx[1] = (value >> 24) & 0xFF;
    tx[2] = (value >> 16) & 0xFF;
    tx[3] = (value >> 8)  & 0xFF;
    tx[4] = value & 0xFF;

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = 0,
        .len = 5,
        .speed_hz = 1000000,
        .bits_per_word = 8,
    };

    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
}

//////////////////////////////////////////////////////
// SPI 讀寄�???
//////////////////////////////////////////////////////
void TMC2240_ReadReg(uint8_t address, uint32_t *value)
{
    uint8_t tx[5] = { address & 0x7F, 0, 0, 0, 0 };
    uint8_t rx[5] = {0};

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 5,
        .speed_hz = 1000000,
        .bits_per_word = 8,
    };

    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);

    // 第�?次傳輸�??��??�數??
    memset(tx, 0, sizeof(tx));
    memset(rx, 0, sizeof(rx));
    tr.tx_buf = (unsigned long)tx;
    tr.rx_buf = (unsigned long)rx;
    ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);

    *value = (rx[1] << 24) | (rx[2] << 16) | (rx[3] << 8) | rx[4];
}

//////////////////////////////////////////////////////
// TMC2240 初始化
//////////////////////////////////////////////////////
int16_t Tmc2240_init(void)
{
    printf("init TMC2240...\n");

    if (spi_init("/dev/spidev0.0", SPI_MODE_3, 1000000) != 0) {
        printf("SPI init error \n");
        return TMC2240_ERROR_SPI;
    }

    uint32_t val;

    TMC2240_WriteReg(0x0A, 0x02);
    usleep(1000);
    TMC2240_ReadReg(0x0A, &val);
//    printf("0x0A = 0x%08X\n", val);

    TMC2240_WriteReg(0x00, 0x00000004);
    usleep(1000);
    TMC2240_ReadReg(0x00, &val);
//    printf("GCONF = 0x%08X\n", val);

    TMC2240_WriteReg(0x0C, 0x00);
    usleep(1000);
    TMC2240_ReadReg(0x0C, &val);
//    printf("0x0C = 0x%08X\n", val);

    TMC2240_WriteReg(0x10, (31 << 16) | (10 << 8) | 16);
    usleep(1000);
    TMC2240_ReadReg(0x10, &val);
//    printf("0x10 = 0x%08X\n", val);

    TMC2240_WriteReg(0x6C, 0x140000C3);                       //�C�@�B�Ӥ�� 16 �ӷL�B
    usleep(1000);
    TMC2240_ReadReg(0x6C, &val);
//    printf("CHOPCONF = 0x%08X\n", val);
        
    if(val != 0x140000C3)  
    {  
       printf("init TMC2240 ERROR : %d \n" , TMC2240_ERROR_REG);   	  	     	 
       return TMC2240_ERROR_REG;
    }   
       
    return 0;
}

//使用 DRV_STATUS 來讀取電流相關資訊
uint32_t TMC2240_ReadCurrent(void)
{
    uint32_t val;
    TMC2240_ReadReg(0x6F, &val); // DRV_STATUS
    
    return val;
}


////////////////////////////////////////////////////////
//// 設�?馬�??�度 
////////////////////////////////////////////////////////
//void Motor_SetSpeed(int pwm_freq)
//{
//
//   gpioSetPWMfrequency(pwm_pin, pwm_freq);
//    
//}

//////////////////////////////////////////////////////
// ?��?馬�?
//////////////////////////////////////////////////////
void Motor_Run(void)
{
	 #ifdef _DEBUG_MOTOR
    printf("Motor_Run \n");
    #endif
    gpioWrite(EN_PIN, 0); // ?��?驅�???   
}

//////////////////////////////////////////////////////
// ?�止馬�?
//////////////////////////////////////////////////////
void Motor_Stop(void)
{
	 #ifdef _DEBUG_MOTOR 
    printf("Motor_Stop \n");
    #endif
    gpioWrite(EN_PIN, 1); // ?��?驅�???    
}

//////////////////////////////////////////////////////
// 清�?
//////////////////////////////////////////////////////
void Motor_Cleanup(void)
{
//    led_thread_running = 0;
//    pthread_join(led_thread, NULL);   //?��?

    if (spi_fd >= 0) {
        close(spi_fd);
        spi_fd = -1;
    }

}


//���F�e�i
void Motor_Foward(void)
{
	gpioWrite(DIR_PIN, 1);
}

//���F��h
void Motor_Back(void)
{
	gpioWrite(DIR_PIN, 0) ;
}

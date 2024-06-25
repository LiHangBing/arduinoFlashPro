#ifndef DEFINES_H
#define DEFINES_H

#define buffSize 64         //缓冲区长度（不能超过串口缓冲区大小 64）

#define ISP_RST   10        //复位引脚可随意，下面为硬件决定
#define ISP_MOSI  11
#define ISP_MISO  12
#define ISP_SCK   13
//Arduino Pro or Pro Mini   11 12 13
//Arduino Leonardo          16 14 15


#define I2C_SCL   A5
#define I2C_SDA   A4
//Arduino Pro or Pro Mini   A5 A4   pullup is needed


#endif

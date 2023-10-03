/*
	该程序基于简易的arduino硬件，主要目的是烧录SPI Flash，如W25QXX等，主要原理是实现了UART转SPI的功能，需配合上位机使用。
    Copyright (C) 2023  LiHangBing

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/





#include <arduino.h>
#include "commands.h"

#define UART_SPEED 1000000
/*
波特率配置源码：
uint16_t baud_setting = (F_CPU / 4 / baud - 1) / 2;   //此处实际+0.5做近似处理，没有错误
可设置的波特率： Fcpu/8/(n+1)   n>=0  测试最大1M可用

 */

int CMD;

void setup() {
  Serial.begin(UART_SPEED);       //串口配置（arduino的USB虚拟串口存在一些问题，需要先用arduino打开一次串口）
  Serial.setTimeout(1000);        //等待数据流的最大时间间隔 ms 若稳定性差试试提高这个间隔
}

void loop() {
  
  if (Serial.available() > 0) {
    CMD = Serial.read();            //从设备接收到数据中读取一个字节的数据。
    ParseCommand(CMD);              //解析命令
  }
}

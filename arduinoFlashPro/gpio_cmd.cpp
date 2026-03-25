/*
	该程序基于简易的arduino硬件，主要目的是烧录SPI Flash，如W25QXX等，主要原理是实现了UART转SPI的功能，需配合上位机使用。
  也可烧录eepron，如24cxx
  配置8路GPIO用于辅助
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
#include "defines.h"
#include "gpio_cmd.h"
#include "commands.h"

extern byte buff[buffSize];

//30 GPIO初始化 ----------------------------------------------
void gpio_init() {
  long i2c_speed;
  byte bytesread;

  bytesread = Serial.readBytes(buff, 2);      //从串口读取2字节，指示输入输出及上下拉，共8个IO。
  if (bytesread != 2) {
    Serial.write(ERROR_TIMOUT); //超时
    Serial.flush();
    return;
  }

  for(byte i = 0;i < 8; i++)
  {
    if(buff[0] & (1 << i) == 0)        //第一字节,bit=0输出，bit=1输入
    {
      pinMode(i, OUTPUT);
    }
    else
    {
      if(buff[1] & (1 << i))      //第二字节,bit=0下拉，bit=1上拉
        pinMode(i, INPUT_PULLUP);
      else
        pinMode(i, OUTPUT);
    }
  }
  
  Serial.write(FUNC_GPIO_INIT);    //回传cmd给串口
  Serial.flush();
}

//31 关闭GPIO ----------------------------------------------
void gpio_deinit() {
  for(byte i = 0;i < 8; i++)
    pinMode(i, INPUT_PULLUP);     //默认切换回输入模式

  Serial.write(FUNC_I2C_DEINIT); //回传cmd给串口
  Serial.flush();
}


//32 IO读
void gpio_read(){
  byte bytesret = 0;

  for(byte i = 0;i < 8; i++)
    bytesret |= digitalRead(i) ? (1 << i) : 0;
  
  Serial.write(bytesret); //回传IO
  Serial.write(FUNC_GPIO_READ);   //回传命令码
  Serial.flush();
}


//33 IO写
void gpio_write()
{
  byte bytesread;

  bytesread = Serial.readBytes(buff, 1);      //从串口读取1字节，指示写入IO

  if (bytesread == 0) {
    Serial.write(ERROR_TIMOUT); //超时
    Serial.flush();
    return;
  }

  for(byte i = 0;i < 8; i++)
    digitalWrite(i, buff[0] & (1 << i) ? HIGH : LOW);

  Serial.write(FUNC_GPIO_WRITE); //回传cmd给串口
  Serial.flush();
}

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
#include <SPI.h>
#include "defines.h"
#include "spi_cmd.h"
#include "commands.h"

extern byte buff[buffSize];

//7 SPI初始化 ----------------------------------------------
void spi_cmd_init() {
  long spi_speed;
  byte bytesread;
  
  bytesread = Serial.readBytes(buff, 1);      //从串口读取1字节，指示SPI时钟速度
  if (bytesread == 0) {
    Serial.write(ERROR_TIMOUT); //超时
    Serial.flush();
    return;
  }

  switch(buff[0])   //SPI时钟速度（Arduino时钟16MHz，分频可选2, 4, 8, 16, 32, 64，128）
  {
    case 2:
    case 4:
    case 8:
    case 16:
    case 32:
    case 64:
    case 128:
      spi_speed = F_CPU / buff[0];
      break;

    default:
      Serial.write(ERROR_RECV); //接收错误
      Serial.flush();
      return;
  }

  SPI.begin();
  SPI.beginTransaction(SPISettings(spi_speed, MSBFIRST, SPI_MODE0));  //设置SPI速度，发送时序
  pinMode(ISP_RST, OUTPUT);     //CE引脚
  

  Serial.write(FUNC_SPI_INIT); //回传cmd给串口（7）
  Serial.flush();
}

//8 关闭SPI -----------------------------------------
void spi_cmd_deinit() {
  SPI.endTransaction();
  SPI.end();
  pinMode(ISP_RST, INPUT);
  
  Serial.write(FUNC_SPI_DEINIT); //回传cmd给串口（8）
  Serial.flush();
}

//9  spi拉低CE引脚 -----------------------------------------
void spi_cmd_ce()
{
  digitalWrite(ISP_RST, LOW);     //拉低CS引脚
  Serial.write(FUNC_SPI_CE); //回传命令码
  Serial.flush();
}

//10  spi释放CE引脚 -----------------------------------------
void spi_cmd_dece()
{
  digitalWrite(ISP_RST, HIGH);     //拉低CS引脚
  Serial.write(FUNC_SPI_DECE); //回传命令码
  Serial.flush();
}

//11  spi读命令 -----------------------------------------
void spi_cmd_read() {
  byte bytesread;

  bytesread = Serial.readBytes(buff, 1);  //从串口取1个字节

  if (bytesread == 0)
  {
    Serial.write(ERROR_TIMOUT);
    Serial.flush();
    return;
  }
  
  if (buff[0] > buffSize) {
    Serial.write(ERROR_RECV);
    Serial.flush();
    return;
  }
  bytesread = buff[0];            //要读的数据长度

  Serial.write(FUNC_SPI_READ); //回传命令码
  Serial.flush();

  SPI.transfer(buff, bytesread);     //SPI交换数据（buff写入并保存读取的字节）
  Serial.write(buff, bytesread);   //上传读取的数据
  Serial.flush();         //刷新缓冲区

  Serial.write(FUNC_SPI_READ); //回传命令码
  Serial.flush();
}

//12  spi写命令 -----------------------------------------
void spi_cmd_write() {
  byte byteswrite;
  byte bytesread;
  
  byteswrite = Serial.readBytes(buff, 1);  //从串口取1个字节

  if (byteswrite == 0)
  {
    Serial.write(ERROR_TIMOUT);
    Serial.flush();
    return;
  }
  
  if (buff[0] > buffSize) {
    Serial.write(ERROR_RECV);
    Serial.flush();
    return;
  }
  byteswrite = buff[0];            //要写的数据长度

  Serial.write(FUNC_SPI_WRITE); //回传命令码
  Serial.flush();

  bytesread = Serial.readBytes(buff, byteswrite); //从串口读取要写入的数据
  if (byteswrite != bytesread) {
    Serial.write(ERROR_RECV);
    Serial.flush();
    return;
  }

  SPI.transfer(buff, byteswrite);                 //SPI写入
  Serial.write(FUNC_SPI_WRITE); //回传命令码
  Serial.flush();
}

//13  测试命令，用于连通性测试，也可用于波特率识别
void spi_cmd_tst()
{
  byte bytesread;

  Serial.write(0xa5);
  Serial.write(0x5a);
  Serial.flush();         //发送两个识别码

  bytesread = Serial.readBytes(buff, 2);  //接收两个识别码
  if (bytesread != 2 || buff[0] != 0xa5 || buff[1] != 0x5a) {
    Serial.write(ERROR_RECV);
    Serial.flush();
    return;
  }
  
  Serial.write(FUNC_SPI_TST); //回传命令码
  Serial.flush();
}

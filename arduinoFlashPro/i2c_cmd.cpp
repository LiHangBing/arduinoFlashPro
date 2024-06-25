/*
	该程序基于简易的arduino硬件，主要目的是烧录SPI Flash，如W25QXX等，主要原理是实现了UART转SPI的功能，需配合上位机使用。
  也可烧录eepron，如24cxx
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
#include "src/Wire_new.h"             //i2c支持库
#include "defines.h"
#include "i2c_cmd.h"
#include "commands.h"

extern byte buff[buffSize];

//20 I2C初始化 ----------------------------------------------
void i2c_cmd_init() {
  long i2c_speed;
  byte bytesread;

  bytesread = Serial.readBytes(buff, 1);      //从串口读取1字节，指示SPI时钟速度
  if (bytesread == 0) {
    Serial.write(ERROR_TIMOUT); //超时
    Serial.flush();
    return;
  }

  switch(buff[0])   //I2C时钟速度，可设置为0(standard mode 100K)、1(fast mode 400K). 2(low speed mode 10K), 
  {
    case 0:
      i2c_speed = 100000;
      break;
    case 1:
      i2c_speed = 400000;
      break;
    case 2:
      i2c_speed = 10000;
      break;

    default:
      Serial.write(ERROR_RECV); //接收错误
      Serial.flush();
      return;
  }

  Wire_new.begin();
  Wire_new.setClock(i2c_speed);
  
  Serial.write(FUNC_I2C_INIT);    //回传cmd给串口
  Serial.flush();
}

//21 关闭I2C ----------------------------------------------
void i2c_cmd_deinit() {

  //Wire_new.endTransmission();
  Wire_new.end();

  Serial.write(FUNC_I2C_DEINIT); //回传cmd给串口
  Serial.flush();
}


//i2c起始信号
void i2c_cmd_start(){
  byte bytesret;

  bytesret = Wire_new.sendStart();     //发送起始信号
  
  if(bytesret == 0)
    Serial.write(FUNC_I2C_START); //回传命令码
  else
    Serial.write(ERROR_OPERAT);   //收到NACK或超时
  Serial.flush();
}



//i2c停止信号
void i2c_cmd_stop()
{
  Wire_new.sendStop();

  Serial.write(FUNC_I2C_STOP); //回传cmd给串口
  Serial.flush();
}





//21 I2C读 ----------------------------------------------
void i2c_cmd_read() {
  byte bytesread;
  byte nack_last;
  byte bytesret;

  bytesread = Serial.readBytes(buff, 2);      //从串口读取2字节，指示读取的长度,和最后是否NACK，最大BUFFER_LENGTH（32） 串口缓冲区长度64，因此无需考虑串口缓冲区长度
  if (bytesread != 2) {
    Serial.write(ERROR_TIMOUT); //超时
    Serial.flush();
    return;
  }

  bytesread = buff[0];
  nack_last = buff[1];
  if(bytesread > BUFFER_LENGTH){
    Serial.write(ERROR_RECV);   //命令错误
    Serial.flush();
    return;
  }

  Serial.write(FUNC_I2C_READ); //回传命令码
  Serial.flush();

  bytesret = Wire_new.readData(buff, bytesread, nack_last);     //读数据
  Serial.write(buff, bytesread);   //上传读取的数据
  Serial.flush();         //刷新缓冲区

  if(bytesret == 0)
    Serial.write(FUNC_I2C_READ); //回传命令码
  else
    Serial.write(ERROR_OPERAT);   //收到NACK或超时,读取失败
  Serial.flush();
}

//22 I2C写
void i2c_cmd_write() {
  byte byteswrite;
  byte bytesread;
  byte bytesret;

  bytesread = Serial.readBytes(buff, 1);      //从串口读取1字节，指示写入的长度,最大BUFFER_LENGTH（32） 串口缓冲区长度64，因此无需考虑串口缓冲区长度
  if (bytesread == 0) {
    Serial.write(ERROR_TIMOUT); //超时
    Serial.flush();
    return;
  }

  byteswrite = buff[0];
  if(byteswrite > BUFFER_LENGTH){
    Serial.write(ERROR_RECV);   //命令错误
    Serial.flush();
    return;
  }

  Serial.write(FUNC_I2C_WRITE); //回传命令码
  Serial.flush();

  bytesread = Serial.readBytes(buff, byteswrite); //从串口读取要写入的数据
  if (byteswrite != bytesread) {
    Serial.write(ERROR_RECV);
    Serial.flush();
    return;
  }

  bytesret = Wire_new.writeData(buff, byteswrite);     //写入数据，并获取ack

  if(bytesret == 0)
    Serial.write(FUNC_I2C_WRITE); //回传命令码
  else
    Serial.write(ERROR_OPERAT);   //收到NACK或超时，i2c写入失败
  Serial.flush();
}


//28 测试命令，用于连通性测试
void i2c_cmd_tst()
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
  
  Serial.write(FUNC_I2C_TST); //回传命令码
  Serial.flush();
}

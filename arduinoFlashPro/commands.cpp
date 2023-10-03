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
#include "spi_cmd.h"
#include "defines.h"

byte buff[buffSize];

void ParseCommand(char cmd) {
  //spi 读、写、初始化、解除初始化
  switch(cmd)
  {
    case FUNC_SPI_INIT:
      spi_cmd_init();
      break;

    case FUNC_SPI_DEINIT:
      spi_cmd_deinit();
      break;

    case FUNC_SPI_CE:
      spi_cmd_ce();
      break;

    case FUNC_SPI_DECE:
      spi_cmd_dece();
      break;

    case FUNC_SPI_READ:
      spi_cmd_read();
      break;

    case FUNC_SPI_WRITE:
      spi_cmd_write();
      break;

    case FUNC_SPI_TST:
      spi_cmd_tst();
      break;

    default:
      Serial.write(ERROR_NO_CMD);     //错误的cmd 100
      Serial.flush();
  }
}

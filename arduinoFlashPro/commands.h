#ifndef COMMANDS_H
#define COMMANDS_H


//上位机传送的SPI命令码
#define FUNC_SPI_INIT 7
#define FUNC_SPI_DEINIT 8
#define FUNC_SPI_CE 9
#define FUNC_SPI_DECE 10
#define FUNC_SPI_READ 11
#define FUNC_SPI_WRITE 12
#define FUNC_SPI_TST 13

//上位机传送的I2C命令码
#define FUNC_I2C_INIT      20
#define FUNC_I2C_DEINIT      21
#define FUNC_I2C_READ      22
#define FUNC_I2C_WRITE     23
#define FUNC_I2C_START     24
#define FUNC_I2C_STOP      25
#define FUNC_I2C_TST       28

#define ERROR_OPERAT 97
#define ERROR_TIMOUT 98
#define ERROR_RECV 99
#define ERROR_NO_CMD 100



void ParseCommand(char cmd);


#endif

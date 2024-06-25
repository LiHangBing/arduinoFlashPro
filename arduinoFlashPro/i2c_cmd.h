#ifndef I2C_CMD_H
#define I2C_CMD_H


void i2c_cmd_init();
void i2c_cmd_deinit();


void i2c_cmd_start();
void i2c_cmd_stop();
void i2c_cmd_read();
void i2c_cmd_write();
void i2c_cmd_tst();



//void i2c_cmd_ack();



#endif
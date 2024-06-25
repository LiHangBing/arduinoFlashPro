/*
  TwoWire_new.cpp - TWI/I2C library for Wiring & Arduino
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
  Modified 2012 by Todd Krein (todd@krein.org) to implement repeated starts
  Modified 2017 by Chuck Todd (ctodd@cableone.net) to correct Unconfigured Slave Mode reboot
  Modified 2020 by Greyson Christoforo (grey@christoforo.net) to implement timeouts
*/

extern "C" {
  #include <stdlib.h>
  #include <string.h>
  #include <inttypes.h>
  #include <util/delay.h>
  #include <compat/twi.h>			//添加寄存器定义
  #include "twi/twi.h"
}

#include "Wire_new.h"

// Initialize Class Variables //////////////////////////////////////////////////

uint8_t TwoWire_new::rxBuffer[BUFFER_LENGTH];
uint8_t TwoWire_new::rxBufferIndex = 0;
uint8_t TwoWire_new::rxBufferLength = 0;

uint8_t TwoWire_new::txAddress = 0;
uint8_t TwoWire_new::txBuffer[BUFFER_LENGTH];
uint8_t TwoWire_new::txBufferIndex = 0;
uint8_t TwoWire_new::txBufferLength = 0;

uint8_t TwoWire_new::transmitting = 0;
void (*TwoWire_new::user_onRequest)(void);
void (*TwoWire_new::user_onReceive)(int);

uint32_t I2c_clock = TWI_FREQ;				//I2C时钟频率，默认100K
const uint8_t us_per_loop = 8;	//用于循环延时
#define waitT  50							//最大等待周期（8bit+ack共9个周期，需向上取整）
uint16_t twi_timeout_us = (waitT*1000000 + I2c_clock - 1)/I2c_clock;



// Constructors ////////////////////////////////////////////////////////////////
TwoWire_new::TwoWire_new()
{
}

// Public Methods //////////////////////////////////////////////////////////////

void TwoWire_new::begin(void)				//补丁，不用中断版本，支持再次初始化
{
  rxBufferIndex = 0;
  rxBufferLength = 0;

  txBufferIndex = 0;
  txBufferLength = 0;

  twi_init();
  TWCR = _BV(TWEN) | _BV(TWEA);		//不用中断
  setClock(I2c_clock);				//可能出现再次初始化的情况
  twi_attachSlaveTxEvent(onRequestService); // default callback must exist
  twi_attachSlaveRxEvent(onReceiveService); // default callback must exist
}

void TwoWire_new::begin(uint8_t address)
{
  begin();
  twi_setAddress(address);
}

void TwoWire_new::begin(int address)
{
  begin((uint8_t)address);
}

void TwoWire_new::end(void)
{
  twi_disable();
}

void TwoWire_new::setClock(uint32_t clock)
{
  twi_setFrequency(clock);
  I2c_clock = clock;
  twi_timeout_us = (waitT*1000000 + I2c_clock - 1)/I2c_clock;
}

/***
 * Sets the TWI timeout.
 *
 * This limits the maximum time to wait for the TWI hardware. If more time passes, the bus is assumed
 * to have locked up (e.g. due to noise-induced glitches or faulty slaves) and the transaction is aborted.
 * Optionally, the TWI hardware is also reset, which can be required to allow subsequent transactions to
 * succeed in some cases (in particular when noise has made the TWI hardware think there is a second
 * master that has claimed the bus).
 *
 * When a timeout is triggered, a flag is set that can be queried with `getWireTimeoutFlag()` and is cleared
 * when `clearWireTimeoutFlag()` or `setWireTimeoutUs()` is called.
 *
 * Note that this timeout can also trigger while waiting for clock stretching or waiting for a second master
 * to complete its transaction. So make sure to adapt the timeout to accommodate for those cases if needed.
 * A typical timeout would be 25ms (which is the maximum clock stretching allowed by the SMBus protocol),
 * but (much) shorter values will usually also work.
 *
 * In the future, a timeout will be enabled by default, so if you require the timeout to be disabled, it is
 * recommended you disable it by default using `setWireTimeoutUs(0)`, even though that is currently
 * the default.
 *
 * @param timeout a timeout value in microseconds, if zero then timeout checking is disabled
 * @param reset_with_timeout if true then TWI interface will be automatically reset on timeout
 *                           if false then TWI interface will not be reset on timeout

 */
void TwoWire_new::setWireTimeout(uint32_t timeout, bool reset_with_timeout){
  twi_setTimeoutInMicros(timeout, reset_with_timeout);
}

/***
 * Returns the TWI timeout flag.
 *
 * @return true if timeout has occurred since the flag was last cleared.
 */
bool TwoWire_new::getWireTimeoutFlag(void){
  return(twi_manageTimeoutFlag(false));
}

/***
 * Clears the TWI timeout flag.
 */
void TwoWire_new::clearWireTimeoutFlag(void){
  twi_manageTimeoutFlag(true);
}


uint8_t TwoWire_new::sendStart()			//发送起始信号
{
	if(TW_STATUS == 0)				//因为错误的STOP信号造成的错误状态
		begin();					//暂时方案为重新初始化接口，以支持任意START STOP操作
	
	// send start condition
    TWCR = _BV(TWEN)  | _BV(TWEA) | _BV(TWINT) | _BV(TWSTA);		//使能TWI、清除中断标志、发送起始信号
	uint16_t counter = (twi_timeout_us + us_per_loop - 1)/us_per_loop; // Round up
	while(!(TWCR & _BV(TWINT))){		//Wait for TWINT Flag set. This indicates that the START condition has been transmitted.
	  if (counter > 0ul){
		_delay_us(us_per_loop);
		counter--;
	  } else {
		return 0xff;
	  }
	}
	
	if( TW_STATUS != TW_START && TW_STATUS != TW_REP_START)
		return 0xfe;
	
	//TWCR = _BV(TWEN)  | _BV(TWEA) ;		//发送起始信号后需清除TWSTA位（在地址or数据阶段清除）
	
	return 0;
}

uint8_t TwoWire_new::sendStop()				//发送停止信号
{
	TWCR = _BV(TWEN)  | _BV(TWINT) | _BV(TWSTO);		//使能TWI、清除中断标志、发送停止信号
	return 0;
}

uint8_t TwoWire_new::readData(uint8_t *data, size_t quantity, uint8_t nack_last)	//读取数据（必须先发送地址+读）
{
	uint8_t i;
	
	if(TW_STATUS != TW_MR_SLA_ACK && TW_STATUS != TW_MR_DATA_ACK)	//必须要是 刚发送地址+读 或 接收并ACK上一字节
		return 0xff;
	
	for(i =0; i<quantity; i++)
	{
		if(nack_last != 0 && i==quantity-1)			//最后一字节读取需要判断是否NACK
			TWCR = _BV(TWEN) | _BV(TWINT);							//清除中断标志，启动接收
		else
			TWCR = _BV(TWEN) | _BV(TWINT) | _BV(TWEA);				//清除中断标志，使能ACK，启动接收
		
		uint32_t counter = (twi_timeout_us + us_per_loop - 1)/us_per_loop; // Round up
		while(!(TWCR & _BV(TWINT))){		//Wait for TWINT Flag set.
		  if (counter > 0ul){
			_delay_us(us_per_loop);
			counter--;
		  } else {
			return 0xfe;
		  }
		}
		
		if(TW_STATUS == TW_MR_DATA_ACK || TW_STATUS == TW_MR_DATA_NACK)	//接收完数据并ack/nack
			data[i] = TWDR;
		else
			return 0xfd;
	}
	
	return 0;
}
uint8_t TwoWire_new::writeData(uint8_t *data, size_t quantity)						//写入数据（必须先发送地址+写）
{
	uint8_t i;
	for(i =0; i<quantity; i++)
	{
		TWDR = data[i];				//发数据
		TWCR = _BV(TWEN)  | _BV(TWEA) | _BV(TWINT);		//使能TWI、使能ACK、清除中断标志，清中断标志则立即发送数据
		uint32_t counter = (twi_timeout_us + us_per_loop - 1)/us_per_loop; // Round up
		while(!(TWCR & _BV(TWINT))){		//Wait for TWINT Flag set.
		  if (counter > 0ul){
			_delay_us(us_per_loop);
			counter--;
		  } else
				return 0xff;
		}
		
		if( TW_STATUS != TW_MT_SLA_ACK && TW_STATUS != TW_MT_DATA_ACK && TW_STATUS != TW_MR_SLA_ACK)	//每次发送完都要检查ack（发送地址读/写 ACK，发送数据收到ACK，共三种）
			return 0xfe;
	}
	
	return 0;
}

uint8_t TwoWire_new::requestFrom(uint8_t address, uint8_t quantity, uint32_t iaddress, uint8_t isize, uint8_t sendStop)
{
  if (isize > 0) {
  // send internal address; this mode allows sending a repeated start to access
  // some devices' internal registers. This function is executed by the hardware
  // TWI module on other processors (for example Due's TWI_IADR and TWI_MMR registers)

  beginTransmission(address);

  // the maximum size of internal address is 3 bytes
  if (isize > 3){
    isize = 3;
  }

  // write internal register address - most significant byte first
  while (isize-- > 0)
    write((uint8_t)(iaddress >> (isize*8)));
  endTransmission(false);
  }

  // clamp to buffer length
  if(quantity > BUFFER_LENGTH){
    quantity = BUFFER_LENGTH;
  }
  // perform blocking read into buffer
  uint8_t read = twi_readFrom(address, rxBuffer, quantity, sendStop);
  // set rx buffer iterator vars
  rxBufferIndex = 0;
  rxBufferLength = read;

  return read;
}

uint8_t TwoWire_new::requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop) {
	return requestFrom((uint8_t)address, (uint8_t)quantity, (uint32_t)0, (uint8_t)0, (uint8_t)sendStop);
}

uint8_t TwoWire_new::requestFrom(uint8_t address, uint8_t quantity)
{
  return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)true);
}

uint8_t TwoWire_new::requestFrom(int address, int quantity)
{
  return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)true);
}

uint8_t TwoWire_new::requestFrom(int address, int quantity, int sendStop)
{
  return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)sendStop);
}

void TwoWire_new::beginTransmission(uint8_t address)
{
  // indicate that we are transmitting
  transmitting = 1;
  // set address of targeted slave
  txAddress = address;
  // reset tx buffer iterator vars
  txBufferIndex = 0;
  txBufferLength = 0;
}

void TwoWire_new::beginTransmission(int address)
{
  beginTransmission((uint8_t)address);
}

//
//	Originally, 'endTransmission' was an f(void) function.
//	It has been modified to take one parameter indicating
//	whether or not a STOP should be performed on the bus.
//	Calling endTransmission(false) allows a sketch to 
//	perform a repeated start. 
//
//	WARNING: Nothing in the library keeps track of whether
//	the bus tenure has been properly ended with a STOP. It
//	is very possible to leave the bus in a hung state if
//	no call to endTransmission(true) is made. Some I2C
//	devices will behave oddly if they do not see a STOP.
//
uint8_t TwoWire_new::endTransmission(uint8_t sendStop)
{
  // transmit buffer (blocking)
  uint8_t ret = twi_writeTo(txAddress, txBuffer, txBufferLength, 1, sendStop);
  // reset tx buffer iterator vars
  txBufferIndex = 0;
  txBufferLength = 0;
  // indicate that we are done transmitting
  transmitting = 0;
  return ret;
}

//	This provides backwards compatibility with the original
//	definition, and expected behaviour, of endTransmission
//
uint8_t TwoWire_new::endTransmission(void)
{
  return endTransmission(true);
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
size_t TwoWire_new::write(uint8_t data)
{
  if(transmitting){
  // in master transmitter mode
    // don't bother if buffer is full
    if(txBufferLength >= BUFFER_LENGTH){
      setWriteError();
      return 0;
    }
    // put byte in tx buffer
    txBuffer[txBufferIndex] = data;
    ++txBufferIndex;
    // update amount in buffer   
    txBufferLength = txBufferIndex;
  }else{
  // in slave send mode
    // reply to master
    twi_transmit(&data, 1);
  }
  return 1;
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
size_t TwoWire_new::write(const uint8_t *data, size_t quantity)
{
  if(transmitting){
  // in master transmitter mode
    for(size_t i = 0; i < quantity; ++i){
      write(data[i]);
    }
  }else{
  // in slave send mode
    // reply to master
    twi_transmit(data, quantity);
  }
  return quantity;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire_new::available(void)
{
  return rxBufferLength - rxBufferIndex;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire_new::read(void)
{
  int value = -1;
  
  // get each successive byte on each call
  if(rxBufferIndex < rxBufferLength){
    value = rxBuffer[rxBufferIndex];
    ++rxBufferIndex;
  }

  return value;
}

// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire_new::peek(void)
{
  int value = -1;
  
  if(rxBufferIndex < rxBufferLength){
    value = rxBuffer[rxBufferIndex];
  }

  return value;
}

void TwoWire_new::flush(void)
{
  // XXX: to be implemented.
}

// behind the scenes function that is called when data is received
void TwoWire_new::onReceiveService(uint8_t* inBytes, int numBytes)
{
  // don't bother if user hasn't registered a callback
  if(!user_onReceive){
    return;
  }
  // don't bother if rx buffer is in use by a master requestFrom() op
  // i know this drops data, but it allows for slight stupidity
  // meaning, they may not have read all the master requestFrom() data yet
  if(rxBufferIndex < rxBufferLength){
    return;
  }
  // copy twi rx buffer into local read buffer
  // this enables new reads to happen in parallel
  for(uint8_t i = 0; i < numBytes; ++i){
    rxBuffer[i] = inBytes[i];    
  }
  // set rx iterator vars
  rxBufferIndex = 0;
  rxBufferLength = numBytes;
  // alert user program
  user_onReceive(numBytes);
}

// behind the scenes function that is called when data is requested
void TwoWire_new::onRequestService(void)
{
  // don't bother if user hasn't registered a callback
  if(!user_onRequest){
    return;
  }
  // reset tx buffer iterator vars
  // !!! this will kill any pending pre-master sendTo() activity
  txBufferIndex = 0;
  txBufferLength = 0;
  // alert user program
  user_onRequest();
}

// sets function called on slave write
void TwoWire_new::onReceive( void (*function)(int) )
{
  user_onReceive = function;
}

// sets function called on slave read
void TwoWire_new::onRequest( void (*function)(void) )
{
  user_onRequest = function;
}

// Preinstantiate Objects //////////////////////////////////////////////////////

TwoWire_new Wire_new = TwoWire_new();


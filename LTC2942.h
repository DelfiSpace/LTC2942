/* Code written by Chia Jiun Wei @ 14 Feb 2017
 * <J.W.Chia@tudelft.nl>
 
 * LTC2942: a library to provide high level APIs to interface with the 
 * Linear Technology Gas Gauge. It is possible to use this library in 
 * Energia (the Arduino port for MSP microcontrollers) or in other 
 * toolchains.
 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License 
 * version 3, both as published by the Free Software Foundation.
  
 */

/*!
LTC2942: Battery Gas Gauge with Temperature, Voltage Measurement.

I2C DATA FORMAT (MSB FIRST):

Data Out:
Byte #1                                    Byte #2                     Byte #3

START  SA6 SA5 SA4 SA3 SA2 SA1 SA0 W SACK  C7  C6 C5 C4 C3 C2 C1 C0 SACK D7 D6 D5 D4 D3 D2 D1 D0 SACK  STOP

Data In:
Byte #1                                    Byte #2                                    Byte #3

START  SA6 SA5 SA4 SA3 SA2 SA1 SA0 W SACK  C7  C6  C5 C4 C3 C2 C1 C0 SACK  Repeat Start SA6 SA5 SA4 SA3 SA2 SA1 SA0 R SACK

Byte #4                                   Byte #5
MSB                                       LSB
D15 D14  D13  D12  D11  D10  D9 D8 MACK   D7 D6 D5 D4 D3  D2  D1  D0  MNACK  STOP

START       : I2C Start
Repeat Start: I2c Repeat Start
STOP        : I2C Stop
SAx         : I2C Address
SACK        : I2C Slave Generated Acknowledge (Active Low)
MACK        : I2C Master Generated Acknowledge (Active Low)
MNACK       : I2c Master Generated Not Acknowledge
W           : I2C Write (0)
R           : I2C Read  (1)
Cx          : Command Code
Dx          : Data Bits
X           : Don't care

http://www.linear.com/product/LTC2942
http://www.linear.com/product/LTC2942#demoboards

*/

#ifndef LTC2942_H
#define LTC2942_H

#include <limits.h>
#include "DWire.h"

#define I2C_ADDRESS 			0x64
#define I2C_ALERT_RESPONSE  	0x0C
#define DEVICE_ID				0x00

/*! @name Registers
@{ */
// Registers
#define STATUS_REG                          0x00
#define CONTROL_REG                         0x01
#define ACCUM_CHARGE_MSB_REG                0x02
#define ACCUM_CHARGE_LSB_REG                0x03
#define CHARGE_THRESH_HIGH_MSB_REG          0x04
#define CHARGE_THRESH_HIGH_LSB_REG          0x05
#define CHARGE_THRESH_LOW_MSB_REG           0x06
#define CHARGE_THRESH_LOW_LSB_REG           0x07
#define VOLTAGE_MSB_REG                     0x08
#define VOLTAGE_LSB_REG                     0x09
#define VOLTAGE_THRESH_HIGH_REG             0x0A
#define VOLTAGE_THRESH_LOW_REG              0x0B
#define TEMPERATURE_MSB_REG                 0x0C
#define TEMPERATURE_LSB_REG                 0x0D
#define TEMPERATURE_THRESH_HIGH_REG         0x0E
#define TEMPERATURE_THRESH_LOW_REG          0x0F
//! @}

/*! @name Command Codes
@{ */
// Command Codes
#define AUTOMATIC_MODE                  0xC0
#define MANUAL_VOLTAGE                  0x80
#define MANUAL_TEMPERATURE              0x40
#define SLEEP_MODE                      0x00

#define PRESCALAR_M_1                   0x00
#define PRESCALAR_M_2                   0x08
#define PRESCALAR_M_4                   0x10
#define PRESCALAR_M_8                   0x18
#define PRESCALAR_M_16                  0x20
#define PRESCALAR_M_32                  0x28
#define PRESCALAR_M_64                  0x30
#define PRESCALAR_M_128                 0x38

#define ALERT_MODE                      0x04
#define CHARGE_COMPLETE_MODE            0x02
#define DISABLE_ALCC_PIN                0x00

#define SHUTDOWN_MODE                   0x01

//! @}

/*!
| Conversion Constants                              |  Value   |
| :------------------------------------------------ | :------: |
| LTC2942_CHARGE_lsb                                | 0.085 mAh|
| LTC2942_VOLTAGE_lsb                               | 366.2  uV|
| LTC2942_TEMPERATURE_lsb                           | 0.586   C|
| LTC2942_FULLSCALE_VOLTAGE                         |  6      V|
| LTC2942_FULLSCALE_TEMPERATURE                     | 600     K|

*/
/*! @name Conversion Constants
@{ */
#define CHARGE_lsb 						85			// LSB: 85 microAh
#define VOLTAGE_lsb						0.0003662f
#define TEMPERATURE_lsb					0.25f		/*CHECK*/
#define FULLSCALE_VOLTAGE				6000		//LSB: 6000mV
#define FULLSCALE_TEMPERATURE			600

//! @}

class LTC2942

{
protected:
	DWire &wire;
    unsigned char address;
	unsigned char M;		//prescaler
	unsigned short R_sense;	//Sense resistor
	
public:

	LTC2942(DWire &i2c);
	virtual ~LTC2942( ) {};
	
	unsigned char ping();
	
	// Configure the device
	void init(unsigned short Q, unsigned short R, unsigned short I);
	void reset_charge();
	
	// Retrieve and convert register value to measurements
	unsigned char getVoltage(unsigned short &voltage);
	unsigned char getTemperature(signed short &temperature);
	unsigned char getCharge(unsigned long &coulomb_charge);
	unsigned char getAvailableCapacity(unsigned long &mAh_charge);
	
	
	// read and write from the register
	unsigned char readRegister(unsigned char reg, unsigned char &output);
	unsigned char writeRegister(unsigned char reg, unsigned char val);
	
private:

};

#endif  // LTC2942_H

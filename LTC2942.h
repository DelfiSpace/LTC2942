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

#include "satellite.h"
#include <stdint.h>
#include <stdbool.h>


	bool ltc_readDeviceID(dev_id id);

	// Configure the device
	void init(dev_id id, unsigned short Q, unsigned short R, unsigned short I);
	void reset_charge(dev_id id);

	// Retrieve and convert register value to measurements
  bool ltc_code_to_voltage(dev_id id, uint16_t *voltage);
  bool ltc_code_to_celcius_temperature(dev_id id, int16_t *temperature);
	bool ltc_code_to_millicoulombs(dev_id id,
																 uint16_t R_sense,
																 uint8_t M,
																 uint32_t *coulomb_charge);
	bool ltc_code_to_microAh(dev_id id,
		                       uint16_t R_sense,
													 uint8_t M,
													 uint32_t mAh_charge);

#endif  // LTC2942_H

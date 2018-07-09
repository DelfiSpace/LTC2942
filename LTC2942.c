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

#include "LTC2942.h"
#include "hal_uart.h"

/*! @name LTC2942 I2C Address
@{ */

#define I2C_ADDRESS 			0x64
#define I2C_ALERT_RESPONSE  	0x0C
#define DEVICE_ID				0x00
//! @}

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


/**  Returns the value of the selected internal register
 *
 *   Parameters:
 *   unsigned char reg     register number
 *   unsigned char &       register value
 *
 *   Returns:
 *   unsigned char         0 success
 *                         1 fail
 *
 */
bool ltc_readRegister(dev_id id, uint8_t reg, uint8_t *res)
{

  HAL_I2C_readWrite(id, &reg, 1, res, 1);

  return 0;
}


/**  Sets the value (1 byte) of the selected internal register
 *
 *   Parameters:
 *   unsigned char reg     register number
 *   unsigned char val     register value
 *
 *   Returns:
 *   unsigned char         0 success
 *                         1 fail
 */
bool ltc_writeRegister(dev_id id, uint8_t reg, uint8_t res)
{

    uint8_t tx_buf[2] = { reg, res };

    HAL_I2C_readWrite(id, tx_buf, 2, NULL, 0);
}

/**  Verify if LTC2942 is present
 *
 *   Returns:
 *   true                  LTC2942 is present
 *   false                 otherwise
 *
 */
bool ltc_readDeviceID(dev_id id)
{
	uint8_t res = 0;
	ltc_readRegister(id, STATUS_REG, &res);

  return  (res & 0xC0) == DEVICE_ID;	//Only last 2 bits give device identification, bitmask first 6 bits
}

/**  Initialise the value of control register
 *
 *   Control register is initialise to automatic mode, ALCC pin disable, and prescaler M depending on Q and R
 *
 *   Parameters:
 *   unsigned short Q			   battery capacity in mAh
 *   unsigned short	R			   Sense resistor value in mOhm
 *   unsigned short I			   Max current of the system in mA
 *
 */
void ltc_init(dev_id id) {

  ltc_writeRegister(id, CONTROL_REG, 0xC8);	// M = 4
  usleep(1);
}

/** Reset the accumulated charge count to zero
 */
void ltc_reset_charge(dev_id id)
{
	uint8_t reg_save = 0;	//value of control register

	ltc_readRegister(id, CONTROL_REG, &reg_save);
	ltc_writeRegister(id, CONTROL_REG, (reg_save | SHUTDOWN_MODE));	//shutdown to write into accumulated charge register
	ltc_writeRegister(id, ACCUM_CHARGE_MSB_REG, 0x00);
	ltc_writeRegister(id, ACCUM_CHARGE_LSB_REG, 0x00);
	ltc_writeRegister(id, CONTROL_REG, reg_save);	//Power back on
}



/** Calculate the LTC2942 SENSE+ voltage
 *
 *  Parameters:
 *  unsigned short &				voltage in mV
 *
 *	Returns
 * 	unsigned char         0 success
 *                        1 fail
 *
 *  Note:
 *  1. Datasheet conversion formula divide by 65535, in this library we divide by 65536 (>> 16) to reduce computational load
 *     this is acceptable as the difference is much lower than the resolution of LTC2942 voltage measurement (78mV)
 *  2. Return is in unsigned short and mV to prevent usage of float datatype, the resolution of LTC2942 voltage measurement (78mV),
 *     floating point offset is acceptable as it is lower than the resolution of LTC2942 voltage measurement (78mV)
 *
 */
bool ltc_code_to_voltage(dev_id id, uint16_t *voltage)
{
	uint8_t adc_code[2];

	ltc_readRegister(id, VOLTAGE_MSB_REG, &adc_code[1]);
  usleep(1);
  ltc_readRegister(id, VOLTAGE_LSB_REG, &adc_code[0]);
  usleep(1);

	*voltage = ((adc_code[1] << 8) | adc_code[0]);			//Note: FULLSCALE_VOLTAGE is in mV, to prevent using float datatype
	return 0;
}

/** Calculate the LTC2942 temperature
 *
 *  Parameters:
 *  short &					Temperature in E-2 Celcius
 *
 *	Returns
 * 	unsigned char         0 success
 *                        1 fail
 *
 *  Note:
 *  1. Datasheet conversion formula divide by 65535, in this library we divide by 65536 (>> 16) to reduce computational load
 *     this is acceptable as the difference is much lower than the resolution of LTC2942 temperature measurement (3 Celcius)
 *  2. Return is in short to prevent usage of float datatype, floating point offset is acceptable as it is lower than the resolution of LTC2942 voltage measurement (3 Celcius).
 *     Unit in E-2 Celcius, not in 10^3 as it might cause overflow at high temperature
 *
 */
bool ltc_temp(dev_id id, int16_t *temperature)
{

  uint8_t adc_code[2];
  uint32_t bat_temp=0;

  ltc_readRegister(id, TEMPERATURE_MSB_REG, &adc_code[1]);\
  usleep(1);
  ltc_readRegister(id, TEMPERATURE_LSB_REG, &adc_code[0]);
  usleep(1);

  *temperature = (uint16_t)((adc_code[1] << 8) | adc_code[0]);

  return 0;
}


/** Calculate the LTC2942 charge in microAh
 *
 *  Parameters:
 *  unsigned long &		  Coulomb charge in microAh
 *
 *	Returns
 * 	unsigned char         0 success
 *                        1 fail
 *
 *  Note:
 *  Return is in unsigned long to prevent usage of float datatype
 *  Loss of precision is < than LSB (0.085mAh)
 *
 */
bool ltc_capacity(dev_id id, uint16_t *cap) {
  uint8_t adc_code[2];
  uint16_t adc = 0;

  ltc_readRegister(id, ACCUM_CHARGE_MSB_REG, &adc_code[1]);
  usleep(1);
  ltc_readRegister(id, ACCUM_CHARGE_LSB_REG, &adc_code[0]);
  usleep(1);

  *cap = ((adc_code[1] << 8) | adc_code[0] );

  return 0;
}

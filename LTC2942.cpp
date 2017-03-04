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

/**  LTC2942 class creator function
 *
 *   Parameters:
 *   DWire &i2c             I2C object
 *
 */
LTC2942::LTC2942(DWire &i2c): wire(i2c)
{
    address = I2C_ADDRESS;
}

/**  Verify if LTC2942 is present
 *
 *   Returns:
 *   true                  LTC2942 is present
 *   false                 otherwise
 *
 */
unsigned char LTC2942::ping()
{
	return (readRegister(STATUS_REG) & 0xC0) == DEVICE_ID;	//Only last 2 bits give device identification, bitmask first 6 bits
}

/**  Initialise the value of control register
 *   
 *   Control register is initialise to automatic mode, ALCC pin disable, and prescaler M depending on Q and R
 *
 *   Parameters:
 *   unsigned short Q			   battery capacity in mAh
 *   unsigned short	R			   Sense resistor value in mOhm
 *
 */
void LTC2942::init(unsigned short Q, unsigned short R)
{
	double M;
	unsigned char i = 0;
	
	M = double(Q * R /(50 * 0.085 * 512)); //divide by 2^9 (512), reduce computational load
	
	while ( M > 1.0)
	{
		M /= 2.0;
		i++;
	}
	
	if (i >= 7)
	{
		i = 7;
	}
	
	switch(i)
	{
		case 0: 
		writeRegister(CONTROL_REG, (AUTOMATIC_MODE | PRESCALAR_M_1 | DISABLE_ALCC_PIN));
		break;
		
		case 1: 
		writeRegister(CONTROL_REG, (AUTOMATIC_MODE | PRESCALAR_M_2 | DISABLE_ALCC_PIN));
		break;
		
		case 2: 
		writeRegister(CONTROL_REG, (AUTOMATIC_MODE | PRESCALAR_M_4 | DISABLE_ALCC_PIN));
		break;
		
		case 3: 
		writeRegister(CONTROL_REG, (AUTOMATIC_MODE | PRESCALAR_M_8 | DISABLE_ALCC_PIN));
		break;
		
		case 4: 
		writeRegister(CONTROL_REG, (AUTOMATIC_MODE | PRESCALAR_M_16 | DISABLE_ALCC_PIN));
		break;
		
		case 5: 
		writeRegister(CONTROL_REG, (AUTOMATIC_MODE | PRESCALAR_M_32 | DISABLE_ALCC_PIN));
		break;
		
		case 6: 
		writeRegister(CONTROL_REG, (AUTOMATIC_MODE | PRESCALAR_M_64 | DISABLE_ALCC_PIN));
		break;
		
		case 7: 
		writeRegister(CONTROL_REG, (AUTOMATIC_MODE | PRESCALAR_M_128 | DISABLE_ALCC_PIN));
		break;
	}
		
}

/** Reset the accumulated charge count to zero
 */
void LTC2942::reset_charge()
{
	unsigned char reg_save;	//value of control register
	
	reg_save = readRegister(CONTROL_REG);
	writeRegister(CONTROL_REG, (reg_save | SHUTDOWN_MODE));	//shutdown to write into accumulated charge register
	writeRegister(ACCUM_CHARGE_MSB_REG, 0x00);
	writeRegister(ACCUM_CHARGE_LSB_REG, 0x00);
	writeRegister(CONTROL_REG, reg_save);	//Power back on
}



/** Calculate the LTC2942 SENSE+ voltage
 *
 *  Returns:
 *  unsigned short 				voltage in mV
 *
 *  Note:
 *  1. Datasheet conversion formula divide by 65535, in this library we divide by 65536 (>> 16) to reduce computational load 
 *     this is acceptable as the difference is much lower than the resolution of LTC2942 voltage measurement (78mV)
 *  2. Return is in unsigned short and mV to prevent usage of float datatype, the resolution of LTC2942 voltage measurement (78mV), 
 *     floating point offset is acceptable as it is lower than the resolution of LTC2942 voltage measurement (78mV)
 *
 */
unsigned short LTC2942::code_to_voltage()
{
	unsigned short voltage;
	unsigned short adc_code = -1;
	  
	((unsigned char*)&adc_code)[1] = readRegister(VOLTAGE_MSB_REG);
	((unsigned char*)&adc_code)[0] = readRegister(VOLTAGE_LSB_REG);

	voltage = (adc_code *FULLSCALE_VOLTAGE) >> 16;			//Note: FULLSCALE_VOLTAGE is in mV, to prevent using float datatype
	return(voltage);
}

/** Calculate the LTC2942 temperature
 *
 *  Returns:
 *  short 					Temperature in 10^2 Celcius 
 *
 *  Note:
 *  1. Datasheet conversion formula divide by 65535, in this library we divide by 65536 (>> 16) to reduce computational load 
 *     this is acceptable as the difference is much lower than the resolution of LTC2942 temperature measurement (3 Celcius)
 *  2. Return is in short to prevent usage of float datatype, floating point offset is acceptable as it is lower than the resolution of LTC2942 voltage measurement (3 Celcius).
 *     Unit in 10^2 Celcius, not in 10^3 as it might cause overflow at high temperature
 *
 */
short LTC2942::code_to_celcius_temperature()
{
  short temperature;
  unsigned short adc_code = -1;
  
  ((unsigned char*)&adc_code)[1] = readRegister(TEMPERATURE_MSB_REG);
  ((unsigned char*)&adc_code)[0] = readRegister(TEMPERATURE_LSB_REG);
  
  temperature = short((adc_code *FULLSCALE_TEMPERATURE * 100) >> 16 ) - 27315; //Note: multiply by 100 to convert to 10^2 Celcius, to prevent using float datatype
  return(temperature);
}

/** Calculate the LTC2942 charge in milliCoulombs
 *  
 *  Parameters: 
 *  unsigned short R			sense resistor value in mOhm		
 *  unsigned short M			prescalar value
 *
 *  Returns:
 *  unsigned long 				Coulomb charge in mC
 *
 *  Note:
 *  Return is in unsigned long to prevent usage of float datatype as well as prevent overflow
 *
 */
unsigned long LTC2942::code_to_millicoulombs(unsigned short R, unsigned short M)
{
  unsigned long coulomb_charge;
  
  coulomb_charge = code_to_microAh(R, M) * 3.6;	//1microAh = 3.6 mC
  return(coulomb_charge);
}

/** Calculate the LTC2942 charge in microAh
 *
 *  Parameters:
 *  unsigned short R			sense resistor value in mOhm		
 *  unsigned short M			prescalar value
 *
 *  Returns:	
 *  unsigned long 				Coulomb charge in microAh
 *
 *  Note:
 *  Return is in unsigned long to prevent usage of float datatype 
 *  Loss of precision is < than LSB (0.085mAh)
 *     
 */
unsigned long LTC2942::code_to_microAh(unsigned short R, unsigned short M)
{
  unsigned long mAh_charge;
  unsigned short adc_code = -1;
  
  ((unsigned char*)&adc_code)[1] = readRegister(ACCUM_CHARGE_MSB_REG);
  ((unsigned char*)&adc_code)[0] = readRegister(ACCUM_CHARGE_LSB_REG);
  
  mAh_charge = (unsigned long)(adc_code * CHARGE_lsb * M * 5)/(R * 128) * 10;		//charge in microAh, multiplier of 50 is split to 5 and 10 to prevent unsigned long overflow
  return(mAh_charge);
}





/**  Returns the value of the selected internal register
 *
 *   Parameters:
 *   unsigned char reg     register number
 *
 *   Returns:
 *   unsigned char         register value
 *
 */
unsigned char LTC2942::readRegister(unsigned char reg)
{
    unsigned char ret = -1;
    wire.beginTransmission(address);
    wire.write(reg);

    unsigned char res = wire.requestFrom(address, 1);
    if (res == 1)
    {
		ret = wire.read();
    }

    return ret;
}


/**  Sets the value (1 byte) of the selected internal register
 *   
 *   Parameters:
 *   unsigned char reg     register number
 *   unsigned char val     register value
 *
 */
void LTC2942::writeRegister(unsigned char reg, unsigned char val)
{
    wire.beginTransmission(address);
    wire.write(reg);
    wire.write(val & 0xFF);      
    wire.endTransmission();
}

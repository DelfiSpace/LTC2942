/* Code written by Chia Jiun Wei and Stefano Speretta
 * <J.W.Chia@tudelft.nl>
 * <S.Speretta@tudelft.nl>
 
 * LTC2942: a library to provide high level APIs to interface with the 
 * Linear Technology Gas Gauge. It is possible to use this library in 
 * Energia (the Arduino port for MSP micro-controllers) or in Code
 * Composer Studio.
 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License 
 * version 3, both as published by the Free Software Foundation.
  
 */
#include "LTC2942.h"

/**  LTC2942 class creator function
 *
 *   Parameters:
 *   DWire &i2c             I2C object
 *   unsigned short Q       battery capacity in mAh
 *   unsigned short R       Sense resistor value in mOhm
 *
 */
LTC2942::LTC2942(DWire &i2c, const unsigned short Q, const unsigned short R ):
    i2cBus(i2c)
{
    //TODO: remove the loop in initialization code, replace it with
    // something else to avoid getting stuck in the constructor
    unsigned int k, a;
    M = 7;
    for(k = 128; k > 1; k = k / 2)
    {
        a = 278524 * k / R / 128;
        if (a < (2 * Q))
        {
            break;
        }
        M--;
    }

    // multiplying time 4 numerator and denominator to achieve integer operations
    // Num = 0.085 * 50 * 4 * k
    // Den = 4 * 128 * R
    Num = 17 * k;
    Den = 4 * 128 * R;
    Offset = (65535 * Num / Den) - Q;
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
	unsigned char ret;
	if (readRegister(STATUS_REG, ret) == SUCCESS)
	{
	    //Only last 2 bits give device identification, bitmask first 6 bits
		return  (ret & 0xC0) == DEVICE_ID;
	}
	else
	{
		return 0;
	}
}

/**  Initialize the value of control register
 *   
 *   Control register is initialize to automatic mode, ALCC pin set to charge
 *   completion, and prescaler M depending on Q and R
 *
 */
void LTC2942::init( )
{
    // ADC is acquiring samples autonomously
    // use the CC pin to flag charge completion (done by the battery protection circuit)
    unsigned char configuration = AUTOMATIC_MODE | CHARGE_COMPLETE_MODE;

    switch(M)
	{
		case 0: 
		    configuration |= PRESCALAR_M_1;
		    break;
		
		case 1: 
		    configuration |= PRESCALAR_M_2;
		    break;
		
		case 2: 
		    configuration |= PRESCALAR_M_4;
		    break;
		
		case 3: 
		    configuration |= PRESCALAR_M_8;
		    break;
		
		case 4: 
		    configuration |= PRESCALAR_M_16;
		    break;
		
		case 5: 
		    configuration |= PRESCALAR_M_32;
		    break;
		
		case 6: 
		    configuration |= PRESCALAR_M_64;
		    break;
		
		case 7: 
		default:
		    configuration |= PRESCALAR_M_128;
		    break;
	}
    writeRegister( CONTROL_REG, configuration );
}

/**
 *
 *  Set raw battery charge
 *
 *  Parameters:
 *  unsigned short                  raw battery charge
 *
 *  Returns
 *  unsigned char         0 success
 *                        1 fail
 *
 */
unsigned char LTC2942::setRawCharge(unsigned short val)
{
	unsigned char reg_save;	//value of control register
	unsigned char retCode;

	retCode = readRegister(CONTROL_REG, reg_save);
	retCode |= writeRegister(CONTROL_REG, (reg_save | SHUTDOWN_MODE));	//shutdown to write into accumulated charge register
	retCode |= writeRegister(ACCUM_CHARGE_MSB_REG, ((unsigned char*)&val)[1]);
	retCode |= writeRegister(ACCUM_CHARGE_LSB_REG, ((unsigned char*)&val)[0]);
	retCode |= writeRegister(CONTROL_REG, reg_save);	//Power back on
	
    return retCode;
}

/**
 *
 *  Read raw battery charge
 *
 *  Parameters:
 *  unsigned short &                raw battery charge
 *                                  SHRT_MAX in case of failure
 *
 *  Returns
 *  unsigned char         0 success
 *                        1 fail
 *
 */
unsigned char LTC2942::getRawCharge(unsigned short &val)
{
    unsigned char retCode;

    retCode = readRegister(ACCUM_CHARGE_MSB_REG, ((unsigned char*)&val)[1]);
    retCode |= readRegister(ACCUM_CHARGE_LSB_REG, ((unsigned char*)&val)[0]);

    if (retCode == FAIL)
    {
        val = USHRT_MAX;
    }

    return (retCode);
}

/**
 *
 *  Read battery voltage in mV
 *
 *  Parameters:
 *  unsigned short &				voltage in mV
 *                                  USHRT_MAX in case of failure
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
unsigned char LTC2942::getVoltage(unsigned short &voltage)
{
	unsigned short adc_code;
    unsigned char retCode;

    retCode = readRegister(VOLTAGE_MSB_REG, ((unsigned char*)&adc_code)[1]);
    retCode |= readRegister(VOLTAGE_LSB_REG, ((unsigned char*)&adc_code)[0]);

	if (retCode == FAIL)
	{
	    voltage = USHRT_MAX;
	}
	else
	{
	    voltage = (unsigned short)(((int)adc_code * FULLSCALE_VOLTAGE) >> 16);
	    //Note: FULLSCALE_VOLTAGE is in mV, to prevent using float datatype
	}
	return (retCode);
}

/** Calculate the LTC2942 temperature
 *
 *  Parameters:
 *  signed short &					Temperature in units of 0.1 Celcius
 *                                  SHRT_MAX in case of failure
 *
 *	Returns
 * 	unsigned char         0 success
 *                        1 fail
 *
 *  Note:
 *  1. Datasheet conversion formula divide by 65535, in this library we divide by 65536 (>> 16) to reduce computational load 
 *     this is acceptable as the difference is much lower than the resolution of LTC2942 temperature measurement (3 Celcius)
 *  2. Return is in short to prevent usage of float datatype, floating point offset is acceptable as it is lower than the resolution of LTC2942 voltage measurement (3 Celcius).
 *     Unit of 0.1 Celcius
 *
 */
unsigned char LTC2942::getTemperature(signed short &temperature)
{
  unsigned short adc_code;
  unsigned char retCode;
  
  retCode = readRegister(TEMPERATURE_MSB_REG, ((unsigned char*)&adc_code)[1]);
  retCode |= readRegister(TEMPERATURE_LSB_REG, ((unsigned char*)&adc_code)[0]);
  
  if (retCode == FAIL)
  {
      temperature = SHRT_MAX;
  }
  else
  {
      temperature = ((signed short)((((unsigned int)adc_code) * FULLSCALE_TEMPERATURE) >> 16 )) - 2731;
      //Note: multiply by 100 to convert to 10^2 Celcius, to prevent using float datatype
  }
  return(retCode);
}

/**
 *
 *  Get the available capacity in mAh
 *
 *  Parameters:	
 *  unsigned short &		        Capacity in mAh
 *                                  USHRT_MAX in case of failure
 *
 *	Returns
 * 	unsigned char         0 success
 *                        1 fail
 *     
 */
unsigned char LTC2942::getAvailableCapacity(unsigned short &mAh_charge)
{
  unsigned short adc_code = 0;
  unsigned char retCode;

  retCode = readRegister(ACCUM_CHARGE_MSB_REG, ((unsigned char*)&adc_code)[1]);
  retCode |= readRegister(ACCUM_CHARGE_LSB_REG, ((unsigned char*)&adc_code)[0]);
  
  if (retCode == FAIL)
  {
      mAh_charge = USHRT_MAX;
  }
  else
  {
      // charge in mAh: operation is split with numerator, denominator and offset to
      // allow for integer operations
      mAh_charge = (unsigned short)(((unsigned long)adc_code * Num / Den) - Offset);
  }
  return (retCode);
}

/**  Returns the value of the selected internal register
 *
 *   Parameters:
 *   unsigned char reg              register number
 *   unsigned char &                register value
 *
 *   Returns:
 *   unsigned char        0 success
 *                        1 fail
 *
 */
unsigned char LTC2942::readRegister(unsigned char reg, unsigned char &output)
{
    output = 0xFF;
    i2cBus.beginTransmission(I2C_ADDRESS);
    i2cBus.write(reg);


    //to-do: check if readability of this part is sufficient
    unsigned char res = i2cBus.requestFrom(I2C_ADDRESS, 1);
    if (res == 1)
    {
		output = i2cBus.read();
		return SUCCESS;
    }
	else
	{
		return FAIL;
	}
}


/**  Sets the value (1 byte) of the selected internal register
 *   
 *   Parameters:
 *   unsigned char reg              register number
 *   unsigned char val              register value
 *
 *   Returns:
 *   unsigned char        0 success
 *                        1 fail
 */
unsigned char LTC2942::writeRegister(unsigned char reg, unsigned char val)
{
    i2cBus.beginTransmission(I2C_ADDRESS);
    i2cBus.write(reg);
    i2cBus.write(val & 0xFF);      
    return i2cBus.endTransmission();
}

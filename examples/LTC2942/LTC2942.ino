/* This code test the LTC2942 library
   It gets measurement from LTC2942 Gas Gauge
   Measurement: charge, temperature and voltage
   code written by J.W.Chia @ 8/2/2017
*/

#include <DWire.h>
#include <DSerial.h>
#include <LTC2942.h>

// LTC2942_I2C_ADDRESS 0x64
// Refer LTC2642.h for register address

DWire wire;
DSerial serial;

// Battery capacity: 750mAh,
// Rsense: 33mOhm
LTC2942 LTC_1(wire, 750, 33);

//Parameters
unsigned char ping;             //ping feedback from LTC2942
unsigned char ret;              //return from control register
unsigned short v_batt;          //battery voltage
short temp;                     //temperature measurement
unsigned short charge;          //coulomb charge

void setup() {
  // initialize I2C master
  wire.begin();

  //initialize debug UART
  serial.begin();

  // ensure the serial port is initialized
  delay(20);

  serial.println();
  serial.println("-----------------------------------------------------");
  serial.println("----------------   Gas Gauge Tester    --------------");
  serial.println("-----------------------------------------------------");
  serial.println();

  LTC_1.init();
}

void loop()
{

  //Ping LTC2942
  ping = LTC_1.ping();
  serial.print("Ping response from LTC2942? ");
  if (ping == 1)
  {
    serial.print("Yes");
    serial.println();
  }
  else {
    serial.print("error");
    serial.println();
  }

  delay(200);

  //Read battery voltage from Gas Gauge
  if (LTC_1.getVoltage(v_batt) == 0)
  {
    if (v_batt <= 5000 && v_batt > 0)
    {
      serial.print("The battery voltage is: ");
      serial.print(v_batt, DEC);
      serial.print(" mV");
      serial.println();
    }
    else
    {
      serial.print("Battery voltage measurement exceed range!");
      serial.println();
    }
  }
  else
  {
    serial.println("Fail");
  }

  delay(200);

  //Read temperature from Gas Gauge
  if (LTC_1.getTemperature(temp) == 0)
  {
    if (temp > 0 && temp < 8000)
    {
      serial.print("The temperature is: ");
      serial.print(temp, DEC);
      serial.print(" Celcius * 10^-1");
      serial.println();
    }
    else
    {
      serial.print("Temperature measurement exceed range!");
      serial.println();
    }
  }
  else
  {
    serial.println("Fail");
  }

  delay(200);

  //Read charge from gas gauge in mAh
  if (LTC_1.getAvailableCapacity(charge) == 0)
  {
    serial.print("The Coulomb charge is: ");
    serial.print(charge, DEC);
    serial.print(" mAh");
    serial.println();
  }
  else
  {
    serial.println("Fail");
  }

  delay(200);

  serial.print("--------------------------------------------------");
  serial.println();

}





#include "Utilities.h"
#include "Actron485.h"
#include <Wire.h>                                                       // required by BME280 library
#include <BME280_t.h>    

#define RS485_SERIAL_BAUD 4800
#define RS485_SERIAL_MODE SERIAL_8N1
#define RXD GPIO_NUM_34 //Serial port RX2 pin assignment 
#define TXD GPIO_NUM_12 //Serial port TX2 pin assignment
#define WRITE_ENABLE GPIO_NUM_13 //Enable Out / Disable In

BME280<> BMESensor;     
// char bufout[10];
#define ASCII_ESC 27
#define MYALTITUDE  373

Actron485::Controller actronController = Actron485::Controller(RXD, TXD, WRITE_ENABLE);

unsigned long temperatureReadTime;

void setup() {
  // I2C that connects to temp sensor
  Wire.begin(15,14);                                                      
  BMESensor.begin();   

  // Logging serial
  Serial.begin(115200);

  // We are controlling zone 3
  actronController.zoneControlled[zindex(3)] = true;

  // Set the temperature setpoint
  actronController.zoneSetpoint[zindex(3)] = 22;

  actronController.printOutMode = Actron485::PrintOutMode::AllMessages;
}

void loop() {
  actronController.loop();

  // put your main code here, to run repeatedly:
  unsigned long now = millis();

  if ((now - temperatureReadTime) > 10000) {
    BMESensor.refresh();                                                  // read current sensor data
    // sprintf(bufout,"%c[1;0H",ASCII_ESC);
    // Serial.print(bufout);

    Serial.print("Temperature: ");
    Serial.print(BMESensor.temperature);                                  // display temperature in Celsius
    Serial.println("C");
    actronController.zoneTemperature[zindex(3)] = BMESensor.temperature;

    Serial.print("Humidity:    ");
    Serial.print(BMESensor.humidity);                                     // display humidity in %   
    Serial.println("%");

    Serial.print("Pressure:    ");
    Serial.print(BMESensor.pressure  / 100.0F);                           // display pressure in hPa
    Serial.println("hPa");

    float relativepressure = BMESensor.seaLevelForAltitude(MYALTITUDE);
    Serial.print("RelPress:    ");
    Serial.print(relativepressure  / 100.0F);                             // display relative pressure in hPa for given altitude
    Serial.println("hPa");   

    Serial.print("Altitude:    ");
    Serial.print(BMESensor.pressureToAltitude(relativepressure));         // display altitude in m for given pressure
    Serial.println("m");
    temperatureReadTime = now;
  }

}


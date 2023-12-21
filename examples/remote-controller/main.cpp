#include <Actron485.h>

#define RS485_SERIAL_BAUD 4800
#define RS485_SERIAL_MODE SERIAL_8N1
#define RXD GPIO_NUM_27 //Serial port RX2 pin assignment 
#define TXD GPIO_NUM_26 //Serial port TX2 pin assignment
#define WRITE_ENABLE GPIO_NUM_25 //Enable Out / Disable In

// char bufout[10];
#define ASCII_ESC 27
#define MYALTITUDE  373

Actron485::Controller actronController = Actron485::Controller(RXD, TXD, WRITE_ENABLE);

unsigned long temperatureReadTime;

void setup() {
  // Logging serial
  Serial.begin(115200);

  actronController.printOutMode = Actron485::PrintOutMode::AllMessages;
  
}

void loop() {
  actronController.loop();

  // put your main code here, to run repeatedly:
  unsigned long now = millis();

  if (temperatureReadTime == 0 || (now - temperatureReadTime) > 10000) {
    Serial.println("AC Stats: ");

    for (int i=1; i<6; i++) {
      Serial.print("Zone ");
      Serial.print(i);
      Serial.print(": Set Point ");
      Serial.print(actronController.getZoneSetpointTemperature(i));
      Serial.print("°C, Reading ");
      Serial.print(actronController.getZoneCurrentTemperature(i));
      Serial.print("°C, State ");
      Serial.print(actronController.getZoneOnState(i) ? "ON" : "OFF");
      Serial.println();
    }
    Serial.println();
    

    // Serial.print(BMESensor.temperature);                                  // display temperature in Celsius
    // Serial.println("C");
    // actronController.setZoneCurrentTemperature(3, BMESensor.temperature);

    temperatureReadTime = now;
  }

}


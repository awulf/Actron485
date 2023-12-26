#include <Actron485.h>

#define RS485_SERIAL_BAUD 4800
#define RS485_SERIAL_MODE SERIAL_8N1
#define RXD GPIO_NUM_27 //Serial port RX2 pin assignment 
#define TXD GPIO_NUM_26 //Serial port TX2 pin assignment
#define WRITE_ENABLE GPIO_NUM_25 //Enable Out / Disable In

Actron485::Controller actronController = Actron485::Controller(RXD, TXD, WRITE_ENABLE);

unsigned long temperatureReadTime;
long counter;

void setup() {
  // Logging serial
  Serial.begin(115200);

  actronController.printOutMode = Actron485::PrintOutMode::AllMessages;
}

void loop() {
  actronController.loop();

  // put your main code here, to run repeatedly:
  unsigned long now = millis();

  if (temperatureReadTime == 0 || (now - temperatureReadTime) > 5000) {
    Serial.print("AC Stats: ");
    Serial.println(counter);

    Serial.print("Receiving Data: ");
    Serial.println(actronController.receivingData() ? "YES" : "NO");

    // for (int i=1; i<6; i++) {
    //   Serial.print("Zone ");
    //   Serial.print(i);
    //   Serial.print(": Set Point ");
    //   Serial.print(actronController.getZoneSetpointTemperature(i));
    //   Serial.print("°C, Reading ");
    //   Serial.print(actronController.getZoneCurrentTemperature(i));
    //   Serial.print("°C, State ");
    //   Serial.print(actronController.getZoneOnState(i) ? "ON" : "OFF");
    //   Serial.println();
    // }
    Serial.println();

  Serial.println("==");
    if (counter == 1) {
      Serial.println("Off: ");
      actronController.setOperatingMode(Actron485::OperatingMode::Off);
    } else if (counter == 2) {
      Serial.println("Off Auto: ");
      actronController.setOperatingMode(Actron485::OperatingMode::OffAuto);
    } else if (counter == 3) {
      Serial.println("Off Cool: ");
      actronController.setOperatingMode(Actron485::OperatingMode::OffCool);
    } else if (counter == 4) {
      Serial.println("Off Heat: ");
      actronController.setOperatingMode(Actron485::OperatingMode::OffHeat);
    } else if (counter == 5) {
      Serial.println("Fan Only: ");
      actronController.setOperatingMode(Actron485::OperatingMode::FanOnly);
    } else if (counter == 6) {
      Serial.println("Off: ");
      actronController.setOperatingMode(Actron485::OperatingMode::Off);
     } else if (counter == 7) {
      Serial.println("Auto: ");
      actronController.setOperatingMode(Actron485::OperatingMode::Auto);
    } else if (counter == 8) {
      Serial.println("Cool: ");
      actronController.setOperatingMode(Actron485::OperatingMode::Cool);  
    } else if (counter == 9) {
      Serial.println("Heat: ");
      actronController.setOperatingMode(Actron485::OperatingMode::Heat);  
    }

    temperatureReadTime = now;
    counter++;
  }

}


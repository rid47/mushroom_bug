//Download adafruit unified sensor library if you haven't already---------------------------------------------//
//-----------------------------Including required libraries---------------------------------------------------//
#include "EEPROM.h"
#define EEPROM_SIZE 200

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  Serial.println("");
  Serial.println("\nInitializing EEPROM library\n");
  if (!EEPROM.begin(EEPROM_SIZE)) 
  {
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
    delay(1000);
    ESP.restart();
  }

//  //-----------Clearing EEPROM; uncomment the following loop if u want to clear EEPROM-------------------------//
//
//
    for (int i = 0 ; i < 20; i++) {
    EEPROM.write(i, 0);
    
    }
    
    for (int i = 100 ; i < 130; i++) {
    EEPROM.write(i, 0);
    
    }
    EEPROM.commit();



}





void loop() {
  // put your main code here, to run repeatedly:

}

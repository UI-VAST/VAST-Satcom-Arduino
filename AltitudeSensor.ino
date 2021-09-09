#include <Adafruit_MPL3115A2.h>
#include <Wire.h>

Adafruit_MPL3115A2 Baro = Adafruit_MPL3115A2();

void setup() {
  //Setup arduino
  Serial.begin(9600);
  Serial.println("Adafruit_MPL3115A2 test!");
}

void loop() {
  int delay_num = 1000;
  if(! Baro.begin()){
    Serial.println("Couldn't find sensor");
    return;
  }
  float pascals = Baro.getPressure();
  Serial.print(pascals); Serial.println(" pascals");
 
  float altm = Baro.getAltitude();
  Serial.print(altm); Serial.println(" meters");
 
  float tempC = Baro.getTemperature();
  Serial.print(tempC); Serial.println("*C");
 
  delay(delay_num);
}

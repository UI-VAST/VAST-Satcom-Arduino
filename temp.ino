#include "IridiumSBD.h"
#include "SoftwareSerial.h"
#include <TinyGPS.h>
#include <Adafruit_MPL3115A2.h>
#include <Wire.h>
#include <DallasTemperature.h>


/*
 * BasicSend
 *
 * This sketch sends a "Hello, world!" message from the satellite modem.
 * If you have activated your account and have credits, this message
 * should arrive at the endpoints (delivery group) you have configured
 * (email address or HTTP POST).
 *
 * Assumptions
 *
 * The sketch assumes an Arduino Mega or other Arduino-like device with
 * multiple HardwareSerial ports.  It assumes the satellite modem is
 * connected to Serial1.  Change this as needed.  SoftwareSerial on an Uno
 * works fine as well.
 */

/*
 * Personal wiring notes: 
 * If you are using the Mega, just use Serial 1 (TX to TX1-18, RX to RX1-19).
 * GND to GND. 5Vin to 5V. For Iridium.
 * For GPS, use Serial 3 (TX to RX, RX to TX).  
 */
 
//SoftwareSerial Serial1(13, 12); // RockBLOCK serial port on 18/19, 
#define IridiumSerial Serial1 //If you are using the Mega, just use Serial 1 (TX to TX1-18, RX to RX1-19).
#define ss Serial3 // TX, RX 
#define DIAGNOSTICS false // Change this to see diagnostics
// Data wire is plugged into pin 2 on the Arduino 
#define ONE_WIRE_BUS 2 

// Declare the IridiumSBD object
IridiumSBD modem(IridiumSerial);
TinyGPS gps; 
const int gasPin=A0; //Gas sensor.
Adafruit_MPL3115A2 Baro = Adafruit_MPL3115A2();
/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices  
// (not just Maxim/Dallas temperature ICs) 
OneWire oneWire(ONE_WIRE_BUS); 
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
/********************************************************************/ 

static void smartdelay(unsigned long ms);

void setup()
{
  int signalQuality = -1;
  int err;

  // Start the console serial port
  Serial.begin(115200);
  while (!Serial);

  //For GPS.
  ss.begin(9600);

  // Start the serial port connected to the satellite modem
  IridiumSerial.begin(19200);

  // Begin satellite modem operation
  Serial.println(F("Starting modem..."));
  err = modem.begin();
  if (err != ISBD_SUCCESS)
  {
    Serial.print(F("Begin failed: error "));
    Serial.println(err);
    if (err == ISBD_NO_MODEM_DETECTED)
      Serial.println(F("No modem detected: check wiring."));
    return;
  }
  
  // Example: Print the firmware revision
  char version[12];
  err = modem.getFirmwareVersion(version, sizeof(version));
  if (err != ISBD_SUCCESS)
  {
     Serial.print(F("FirmwareVersion failed: error "));
     Serial.println(err);
     return;
  }
  Serial.print(F("Firmware Version is "));
  Serial.print(version);
  Serial.println(F("."));

  // Example: Print the IMEI
  char IMEI[16];
  err = modem.getIMEI(IMEI, sizeof(IMEI));
  if (err != ISBD_SUCCESS)
  {
     Serial.print(F("getIMEI failed: error "));
     Serial.println(err);
     return;
  }
  Serial.print(F("IMEI is "));
  Serial.print(IMEI);
  Serial.println(F("."));

  // Example: Test the signal quality.
  // This returns a number between 0 and 5.
  // 2 or better is preferred.
  err = modem.getSignalQuality(signalQuality);
  if (err != ISBD_SUCCESS)
  {
    Serial.print(F("SignalQuality failed: error "));
    Serial.println(err);
    return;
  }

  Serial.print(F("On a scale of 0 to 5, signal quality is currently "));
  Serial.print(signalQuality);
  Serial.println(F("."));

}

void loop()
{
  int err;
  bool newData = false;
  // For one second we parse GPS data and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ss.available())
    {
      char c = ss.read();
      // Serial.write(c); // uncomment this line if you want to see the GPS data flowing
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
  }

  if (newData)
  {
    char oTemp[9] = "",
       iTemp[9] = "",
       pasca[9] = "",
       alti[9] = "";
    // 1: Outside Temp(Celsius), 2: Inside Temp(Celcius), 3: Pressure(Pascals), 4: Altitude(Meters)

    float floBuff;  // stores the values before they become a string.
    char pay[81] = "RB0012828"; // the payload sent through Rock Block
      
      char err; // Used in rockblock if/else message sent
  
    Serial.println(" Reading Data "); 
    sensors.requestTemperatures(); // Send the command to get temperature readings 
  
    floBuff = sensors.getTempCByIndex(0); // writes temp from oneProbe to float.
    Serial.print(floBuff); Serial.println(" C");
    dtostrf(floBuff, 7, 2, oTemp);  // function converts float to char array.
  
    if(! Baro.begin()){
      Serial.println("Couldn't find sensor");
      return;
    }  
  
    floBuff = Baro.getTemperature();
    Serial.print(floBuff); Serial.println("*C");
    dtostrf(floBuff, 7, 2, iTemp);
    
    floBuff = Baro.getPressure();
    Serial.print(floBuff); Serial.println(" pascals");
    dtostrf(floBuff, 7, 2, pasca); 
   
    floBuff = Baro.getAltitude();
    Serial.print(floBuff); Serial.println(" meters");
    dtostrf(floBuff, 7, 2, alti); 
  
    strcat(pay, oTemp);
    strcat(pay, " ");
    strcat(pay, iTemp);
    strcat(pay, " ");
    strcat(pay, pasca);
    strcat(pay, " ");
    strcat(pay, alti);

    
    float flat, flon, fgas;
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);
    fgas = analogRead(gasPin);
    char buf[45]="(";
    char lat[10]="";
    char lon[10]="";
    char gas[10]="";

     
    dtostrf(flat, 6, 3, lat); 
    //Serial.print(lat);
    dtostrf(flon, 6, 3, lon); 
    dtostrf(fgas, 6, 1, gas); 
    //Serial.print(lon);
    /*
    strcat(buf, lat);
    strcat(buf, ", ");
    strcat(buf, lon);
    strcat(buf, ")");
    strcat(buf, " ");
    strcat(buf, gas);
    Serial.print(buf);*/
    strcat(pay, " (");
    strcat(pay, lat);
    strcat(pay, ", ");
    strcat(pay, lon);
    strcat(pay, ")");
    strcat(pay, " ");
    strcat(pay, gas);
    Serial.println(pay);
    
    err = modem.sendSBDText(pay);
    if (err != ISBD_SUCCESS)
    {
      Serial.print(F("sendSBDText failed: error "));
      Serial.println(err);
      if (err == ISBD_SENDRECEIVE_TIMEOUT)
        Serial.println(F("Try again with a better view of the sky."));
    }
    else
    {
      Serial.println(F("Hey, it worked!"));
    }
    // Clear the Mobile Originated message buffer
    Serial.println(F("Clearing the MO buffer."));
    err = modem.clearBuffers(ISBD_CLEAR_MO); // Clear MO buffer
    if (err != ISBD_SUCCESS)
    {
      Serial.print(F("clearBuffers failed: error "));
      Serial.println(err);
    }
    //smartdelay(1000);
    delay(60000); //1 min delay. 1*60=60*1000
  }
}

static void smartdelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ss.available())
      gps.encode(ss.read());
  } while (millis() - start < ms);
}

#if DIAGNOSTICS
void ISBDConsoleCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}

void ISBDDiagsCallback(IridiumSBD *device, char c)
{
  Serial.write(c);
}
#endif

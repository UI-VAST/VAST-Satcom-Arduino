/*
 * This program reads temperature and sends it over the rockBlock to the server.
 * Further instruction on wiring found in comments below.
 * 
 * Assembled by: Sat-Comm
 */

#include <IridiumSBD.h>
#include "SoftwareSerial.h"

// Iridium Rockblock Comments
/*
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
 

/*
 * Basic Temp
 * 
 * The sketch assumes an Arduino Mega or other Arduino-like device with
 * multiple HardwareSerial ports.  It assumes the satellite modem is
 * connected to Serial3.  Change this as needed.  SoftwareSerial on an Uno
 * works fine as well.
 */

#include "SoftwareSerial.h"
#include <TinyGPS.h>

/*
 * Personal wiring notes: 
 * If you are using the Mega, just use Serial 1 (TX to TX1-18, RX to RX1-19).
 * GND to GND. 5Vin to 5V. For Iridium.
 * For GPS, use Serial 3 (TX to RX, RX to TX).  
 */

/********************************************************************/
// Thermometer Comments
// The Red and Black Wire on the Probe are hooked to 5V and GND respectivly.
// White Wire leads back to Slot 2.
// Voltage is raised on White Wire with 4.75 K Ohms between from 5V Supply. 
/********************************************************************/
// First we include the libraries
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <Adafruit_MPL3115A2.h>
/********************************************************************/
// Data wire is plugged into pin 2 on the Arduino 
#define ONE_WIRE_BUS 2 
/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices  
// (not just Maxim/Dallas temperature ICs) 
OneWire oneWire(ONE_WIRE_BUS); 
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
/********************************************************************/ 

#define IridiumSerial Serial1 //Used for Mega (TX-18 and RX-19)
#define ss Serial3 // TX, RX
#define DIAGNOSTICS false // Change this to see diagnostics

// Declare the IridiumSBD object
IridiumSBD modem(IridiumSerial);

Adafruit_MPL3115A2 Baro = Adafruit_MPL3115A2();

TinyGPS gps; 
const int gasPin1 = A0; //GAS sensor output pin to Arduino analog A0 pin
const int gasPin2 = A1;
const int gasPin3 = A2;

static void smartdelay(unsigned long ms);
static void print_float(float val, float invalid, int len, int prec);
static void print_int(unsigned long val, unsigned long invalid, int len);
static void print_date(TinyGPS &gps);
static void print_str(const char *str, int len);
bool if_timer(int time);

int livePin = 10;

void setup()
{
  int signalQuality = -1;
  int err;
  
  // Start the console serial port
  Serial.begin(115200);
  Serial.setTimeout(1000);

  // Send test message

 Serial.println("Start Loop"); 
 // Start up the library 
 sensors.begin(); 

 pinMode(livePin, OUTPUT);
 digitalWrite(livePin, LOW);
 
}

void loop()
{  

  bool newData = false,
       overRide = false;
  // For one second we parse GPS data and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ss.available()) // && !newData
    {
      char c = ss.read();
      Serial.write(c); // uncomment this line if you want to see the GPS data flowing
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
  
    float floBuff;
    char pay[95] = "";
      
      char err;
    // call sensors.requestTemperatures() to issue a global temperature 
    // request to all devices on the bus 
  
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

  float flat, flon, 
        fgas1,
        fgas2,
        fgas3;
  unsigned long age;
  gps.f_get_position(&flat, &flon, &age);
  fgas1 = analogRead(gasPin1);
  fgas2 = analogRead(gasPin2);
  fgas3 = analogRead(gasPin3);
  
  char dummy[8] = "";
  char lat[10]="";
  char lon[10] = "";
  char sgas1[8] = "",
       sgas2[8] = "",
       sgas3[8] = "";

  //Serial.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
  //Serial.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);    
  dtostrf(flat, 6, 3, lat); 
  dtostrf(flon, 6, 3, lon); 
  
  dtostrf(fgas1, 6, 1, sgas1); 
  dtostrf(fgas2, 6, 1, sgas2);
  dtostrf(fgas3, 6, 1, sgas3);

  strcat(pay, " ");
  strcat(pay, sgas1);
  strcat(pay, " ");
  strcat(pay, sgas2);
  strcat(pay, " ");
  strcat(pay, sgas3);
  
  strcat(pay, " (");
  strcat(pay, lat);
  strcat(pay, ", ");
  strcat(pay, lon);
  strcat(pay, ")");

  Serial.println(pay);
  /*
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
*/


  Serial.println("All Sent, Waiting on Loop");
  //smartdelay(1000);
  digitalWrite(livePin, HIGH);
  delay(10);
  digitalWrite(livePin, LOW);
  delay(60000); //1 min delay.
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

static void print_float(float val, float invalid, int len, int prec)
{
  if (val == invalid)
  {
    while (len-- > 1)
      Serial.print('*');
    Serial.print(' ');
  }
  else
  {
    Serial.print(val, prec);
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i)
      Serial.print(' ');
  }
  smartdelay(0);
}

static void print_int(unsigned long val, unsigned long invalid, int len)
{
  char sz[32];
  if (val == invalid)
    strcpy(sz, "*******");
  else
    sprintf(sz, "%ld", val);
  sz[len] = 0;
  for (int i=strlen(sz); i<len; ++i)
    sz[i] = ' ';
  if (len > 0) 
    sz[len-1] = ' ';
  Serial.print(sz);
  smartdelay(0);
}

static void print_date(TinyGPS &gps)
{
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  if (age == TinyGPS::GPS_INVALID_AGE)
    Serial.print("********** ******** ");
  else
  {
    char sz[32];
    sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d ",
        month, day, year, hour, minute, second);
    Serial.print(sz);
  }
  print_int(age, TinyGPS::GPS_INVALID_AGE, 5);
  smartdelay(0);
}

static void print_str(const char *str, int len)
{
  int slen = strlen(str);
  for (int i=0; i<len; ++i)
    Serial.print(i<slen ? str[i] : ' ');
  smartdelay(0);
}

bool if_timer(int time)
{
  delay(time);
  return 1;
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

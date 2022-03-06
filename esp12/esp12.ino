
/* Testest with
 -  Arduino IDE 1.8.15
    - Arduino AVR board 1.8.3
    - ESP8266 2.7.0

https://arduino.esp8266.com/stable/package_esp8266com_index.json
board: NodeMCU1.0 (ESP-12E Module)

Install NTPClient from manage libraries
    
*/

//Libraries
#include <EEPROM.h>//https://github.com/esp8266/Arduino/blob/master/libraries/EEPROM/EEPROM.h
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <SoftwareSerial.h>;

// This is for each variable to use it's real size when stored
#pragma pack(push, 1)

// pins mapping

/*
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;x``
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;
*/

//Constants
#define EEPROM_SIZE 4 * 1024 * 1024
#define LED D4

// prototipes
void initNtp();
void FlushStoredData();
void readEEPROM();
void registerNewReading();
void blinkLed();
void fakeWrite();
void connectIfNeeded();

const char* ssid = "Starlink";
const char* password = "82111847";
const char* baseURL = "http://94.177.253.187:8888/";

unsigned int address = 0;
unsigned int tConnect = millis();
unsigned long tLastConnectionAttempt = 0;

unsigned long tBoot = millis();

// variables for sensor
 
const int US100_RX = D5;
const int US100_TX = D6;

// To communicate with us-100
SoftwareSerial US100Serial(US100_RX, US100_TX);
 
unsigned int MSByteDist = 0;
unsigned int LSByteDist = 0;
unsigned int mmDist = 0;
int temp = 0;


// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

int counter = 0; // number of registers in EEPROM
unsigned long epochTime = 0;

// Timers
#define NUM_TIMERS 6 // As I can't find an easy way to get the number
                     // of elements in an array

struct
{
    boolean enabled;
    unsigned long timer;
    unsigned long lastRun;
    void (*function)();
    char* functionName;
} TIMERS[] = {
  { true, 30*1000, 0, &FlushStoredData, "FlushStoredData" },
  { false, 10*1000, 0, &readEEPROM, "readEEPROM" },
  { true, 3600*1000, 0, &initNtp, "initNtp" },
  { true, 5*1000, 0, &registerNewReading, "registerNewReading" },  
  { true, 1*1000, 0, &blinkLed, "blinkLed" },    
  //{ false, 10*1000, 0, &fakeWrite, "fakeWrite" },  
  { true, 5*1000, 0, &connectIfNeeded, "connectIfNeeded" },  
};

typedef struct
{
  unsigned long timestamp;
  byte temperature;
  short int value;
} Reading;


void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(115200); 
  EEPROM.begin(EEPROM_SIZE);
  resetEEPROM();
  // sensor
  US100Serial.begin(9600);
}


void registerNewReading(){
  byte temperature;
  short int value;
  char buffer[100];

  fakeRead(&temperature, &value);

  if (epochTime){
    unsigned long now = epochTime + int(millis()/1000);
    sprintf(buffer, "%s/add/%d:%d:%d", baseURL, now, temperature, value);

    // try to send to the server
    // if fails, store locally for further retrying
    if (send(buffer)){
      sprintf(buffer, "Sent %d:%d:%d", now, temperature, value);
      Serial.println(buffer);
    }
    else{
      writeReading(now, temperature, value);
      sprintf(buffer, "Locally stored %d:%d:%d", now, temperature, value);
      Serial.println(buffer);
    }
        
  }

}


void fakeRead(byte* temperature, short int *value){

  *temperature = random(10, 50);
  *value = random(250, 1300);

}

short int mmToLitres(int milimetres){
  return milimetres;
}

void readSensor(byte *temperature, short int *value){
   
    US100Serial.flush();
    US100Serial.write(0x55); 
 
    delay(100);
 
    if(US100Serial.available() >= 2) 
    {
        MSByteDist = US100Serial.read(); 
        LSByteDist = US100Serial.read();
        mmDist  = MSByteDist * 256 + LSByteDist; 
        if((mmDist > 1) && (mmDist < 10000)) 
        {
            Serial.print("Distance: ");
            Serial.print(mmDist, DEC);
            Serial.println(" mm");
        }
    }
 
    US100Serial.flush(); 
    US100Serial.write(0x50); 
 
    delay(100);
    if(US100Serial.available() >= 1) 
    {
        temp = US100Serial.read();
        if((temp > 1) && (temp < 130)) // temprature is in range
        {
            temp -= 45; // correct 45º offset
            Serial.print("Temp: ");
            Serial.print(temp, DEC);
            Serial.println(" ºC.");
        }
    }

    *temperature = (byte) temp;
    *value = mmToLitres(mmDist);
   
}

void FlushStoredData(){

  // You have to start server2.py at bitelxux.tk/sensor.agua

  int sent = 0;
  
  Serial.println("[FLUSH_STORED_DATA] Sending Stored Data");

  if (WiFi.status() != WL_CONNECTED){
    Serial.println("[FLUSH_STORED_DATA] Skipping. I'm not connected to the internet :-/.");
    return;
  }

  Reading reading;  
  unsigned short int counter = readEEPROMCounter();
  unsigned short int cursor = 0;
  int regAddress;
  short int value;

  int regSize = sizeof(reading);

  char buffer[100];
  //sprintf(buffer, "[FLUSH_STORED_DATA] %d registers", counter);
  //Serial.println(buffer);
  
  for (cursor = 0; cursor < counter; cursor++){
      regAddress = 2 + cursor*regSize; // +2 is to skip the counter bytes
      EEPROM.get(regAddress, reading);

      // if value is -1 that register was already sent
      if (reading.value == -1){
        continue;
      }

      sent++;
      
      sprintf(buffer, "%s/add/%d:%d:%d", baseURL, reading.timestamp, reading.temperature, reading.value);

      if (!send(buffer)){
        Serial.print("Error sending ");
        Serial.println(cursor);
      }
      else
      {
        Serial.print("[FLUSH_STORED_DATA] Success sending record ");
        Serial.println(cursor);
        // We don't want to write the whole struct to save write cycles
        value = -1;
        EEPROM.put(regAddress + sizeof(reading.timestamp) + sizeof(reading.temperature), value);
        EEPROM.commit();            
      }
  }

  Serial.print("[FLUSH_STORED_DATA] ");
  if (sent){
     Serial.print(sent);
     Serial.println(" records sent");
  }
  else{
    Serial.println("Nothing left to send");
  }

}

void initNtp(){
  // Initialize a NTPClient to get time
  Serial.println("[NTP_UPDATE] Updating NTP time");
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(7200);  
  timeClient.update();
  epochTime = timeClient.getEpochTime();
  Serial.print("epochTime set to ");
  Serial.println(epochTime);
}


void readEEPROM(){
  
  unsigned short int counter = readEEPROMCounter();
  unsigned short int cursor = 0;
  int regAddress;
  Reading reading;

  int regSize = sizeof(reading);

  Serial.print(counter);
  Serial.println(" registers");

  /*
  for (cursor = 0; cursor < counter; cursor++){
    regAddress = 2 + cursor*regSize; // +2 is to skip the counter bytes
    EEPROM.get(regAddress, reading);
    Serial.print("Address ");
    Serial.print(regAddress);
    Serial.print(" : ");
    Serial.print(reading.timestamp);
    Serial.print(":");
    Serial.print(reading.temperature);    
    Serial.print(":");
    Serial.println(reading.value);        
  }
  */
  
}

void resetEEPROM(){
  
  int address = 0;
  unsigned short int counter=0;
  EEPROM.put(address, counter);
  EEPROM.commit();  
  int a = 1/0;
}

unsigned short int readEEPROMCounter(){
  unsigned short int counter;
  EEPROM.get(0, counter);
  return counter;
}

void writeReading(unsigned long in_timestamp, byte in_temperature, short int in_value){
  
  int address;
  int regAddress;
  unsigned short int counter = readEEPROMCounter();
  unsigned short int cursor;
 
  Reading newReading;
  Reading currentReading;
  
  newReading.timestamp = in_timestamp;
  newReading.temperature = in_temperature;
  newReading.value = in_value;

  int regSize = sizeof(newReading);
  
  for (cursor = 0; cursor < counter; cursor++){
    regAddress = sizeof(counter) + cursor*regSize; // +sizeof counter is to skip the counter bytes
    EEPROM.get(regAddress, currentReading);

    if (currentReading.value == -1){ // Re-use old register
      Serial.print("Reused position ");
      Serial.println(regAddress);
      EEPROM.put(regAddress, newReading);
      if (!EEPROM.commit()) {
        Serial.println("Commit failed on re-use");
      }
      break;
    }
  }

  if (cursor == counter){ // new register
      regAddress = sizeof(counter) + cursor*regSize; // +sizeof counter is to skip the counter bytes

      // Serial.print("New register at ");
      // Serial.println(regAddress);

      EEPROM.put(regAddress, newReading);
      counter += 1;
      EEPROM.put(0, counter); 
      if (!EEPROM.commit()) {
        Serial.println("Commit failed on new register");
      }
  }
  
}

void attendTimers(){
    
  for (int i=0; i<NUM_TIMERS; i++){
    if (TIMERS[i].enabled && millis() - TIMERS[i].lastRun >= TIMERS[i].timer) {
      TIMERS[i].function();
      TIMERS[i].lastRun = millis();
    }
  }
  
}

void connect(){
  Serial.println("");
  Serial.println("Connecting");

  WiFi.begin(ssid, password);

  tLastConnectionAttempt = millis();
  tConnect =  millis();
  while(WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED, !digitalRead(LED));
    delay(100);
    if ((millis() - tConnect) > 500){
      Serial.print(".");
      tConnect = millis();
    }

    // If it doesn't connect, let the thing continue
    // in the case that in a previous connection epochTime was
    // initizalized, it will store readings for future send
    if (millis() - tLastConnectionAttempt >= 30000L){      
      break;
    }    
  }

  Serial.println("");

  if (WiFi.status() == WL_CONNECTED) { 
    initNtp();
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());  
  }
  else
  {
    Serial.println("Failed to connect");
  }

  tLastConnectionAttempt = millis();
  
}

void blinkLed(){
  digitalWrite(LED, !digitalRead(LED));
}

bool send(String what){
  bool result;
  WiFiClient client;
  HTTPClient http;
  http.begin(client, what.c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200){
        /*
        String payload = http.getString();
        Serial.print("[");
        Serial.print(httpResponseCode);
        Serial.print("] ");
        Serial.println(payload);
        */
        result = true;
      }
      else {
        Serial.print("[send] Error code: ");
        Serial.println(httpResponseCode);
        result = false;
      }
      // Free resources
      http.end();    

      return result;
}


void connectIfNeeded(){
  // If millis() < 30000L is the first boot so it will try to connect
  // for further attempts it will try with spaces of 60 seconds

  if (WiFi.status() != WL_CONNECTED && (millis() < 30000L || millis() - tLastConnectionAttempt > 60000L)){
    Serial.println("Trying to connect");
    connect();
  }  

  // also if we don't have time, try to update
  if (!epochTime){
    Serial.println("NTP time not updated. Trying to");
    initNtp();
  }
}

void loop() {

  attendTimers();
  delay(20);

  //digitalWrite(LED, !digitalRead(LED));
  //delay(100);

}

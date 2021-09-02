//Libraries
#include <EEPROM.h>//https://github.com/esp8266/Arduino/blob/master/libraries/EEPROM/EEPROM.h
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#pragma pack(push, 1)
// without this, the struct Reading would use 8 bytes instead of 6.
// it would pack the unsigned short in 4 bytes when it's size is 2 bytes.

//Constants
#define EEPROM_SIZE 4 * 1024 * 1024
#define LED 2

// prototipes
void initNtp();
void sendStoredData();
void readEEPROM();

const char* ssid = "cnn";
const char* password = "kkkkkkkk";
String baseURL = "http://94.177.253.187:8888/";

unsigned int address = 0;
unsigned int tConnect = millis();
unsigned long tLastConnectionAttempt = 0;

unsigned long tBoot = millis();

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

int counter = 0;
unsigned long epochTime = 0;
long tBlink = millis();

// Timers
//#define TIMER 0
//#define LAST_RUN 1
#define NUM_TIMERS 3

struct
{
    boolean enabled;
    unsigned long timer;
    unsigned long lastRun;
    void (*function)();
    char* functionName;
} TIMERS[] = {
  { true, 15*1000, 0, &sendStoredData, "sendStoredData" },
  { true, 10*1000, 0, &readEEPROM, "readEEPROM" },
  { true, 3600*1000, 0, &initNtp, "initNtp" },
};

typedef struct
{
  unsigned long timestamp;
  short int value;
} Reading;


void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(115200); 
  EEPROM.begin(EEPROM_SIZE);
  //resetEEPROM();
}


void sendStoredData(){

  
  Serial.println("[SEND_STORED_DATA] Sending Stored Data");

  if (WiFi.status() != WL_CONNECTED){
    Serial.println("[SEND_STORED_DATA] I'm not connected to the internet :-/.");
    return;
  }

  Reading reading;  
  unsigned short int counter = readEEPROMCounter();
  unsigned short int cursor = 0;
  int regAddress;
  short int value;

  int regSize = sizeof(reading);

  char buffer[100];
  sprintf(buffer, "%d registers", counter);
  Serial.println(buffer);
  
  for (cursor = 0; cursor < counter; cursor++){
    regAddress = 2 + cursor*regSize; // +2 is to skip the counter bytes
    EEPROM.get(regAddress, reading);

    if (reading.value != -1){
      if (random(100) < 5){
        Serial.print("Error sending ");
        Serial.println(cursor);
      }
      else
      {
        Serial.print("SUCCESS sending ");
        Serial.println(cursor);
        value = -1;
        EEPROM.put(regAddress + sizeof(reading.timestamp), value);
        EEPROM.commit();            
      }
    }
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
}

void readEEPROM(){
  
  unsigned short int counter = readEEPROMCounter();
  unsigned short int cursor = 0;
  int regAddress;
  Reading reading;

  int regSize = sizeof(reading);

  Serial.print(counter);
  Serial.println(" registers");
  
  for (cursor = 0; cursor < counter; cursor++){
    regAddress = 2 + cursor*regSize; // +2 is to skip the counter bytes
    EEPROM.get(regAddress, reading);
    Serial.print("Address ");
    Serial.print(regAddress);
    Serial.print(" : ");
    Serial.print(reading.timestamp);
    Serial.print(":");
    Serial.println(reading.value);    
  }
  
}


void resetEEPROM(){
  
  int address = 0;
  unsigned short int counter=0;
  EEPROM.put(address, counter);
  EEPROM.commit();  
}

unsigned short int readEEPROMCounter(){
  unsigned short int counter;
  EEPROM.get(0, counter);
  return counter;
}

void writeReading(unsigned long in_timestamp, short int in_value){
  
  int address;
  int regAddress;
  unsigned short int counter = readEEPROMCounter();
  unsigned short int cursor;
 
  Reading newReading;
  Reading currentReading;
  
  newReading.timestamp = in_timestamp;
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

      Serial.print("New register at ");
      Serial.println(regAddress);

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
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting");

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

void blink(){
  digitalWrite(LED, LOW);
  delay(500);
  digitalWrite(LED, HIGH);
  delay(500);
}

bool send(String what){
  bool result;
  WiFiClient client;
  HTTPClient http;
  http.begin(client, what.c_str());
  int httpResponseCode = http.GET();
  if (httpResponseCode>0){
        String payload = http.getString();
        Serial.print("[");
        Serial.print(httpResponseCode);
        Serial.print("] ");
        Serial.println(payload);
        result = true;
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
        result = false;
      }
      // Free resources
      http.end();    

      return result;
}

void storeReading(String what){
  // todo
  Serial.print("Fallo enviando ");
  Serial.println(what);
}

void read(){
  // Instead of updating the time everytime, it's initialized and then
  // we add millis()/1000 to update it.
  // If epochTime was initialized, try to send
  // If send fails, store for future attempt
  // If epochTime hasn't been initializad, reading is discarted
  unsigned long now = epochTime + int(millis()/1000);
  String what = (String) now + ":" + (String) counter++;
  if (epochTime && !send(baseURL + "add/" + what)){
    storeReading(what);
  }
  
}

void connectIfNeeded(){
  // If millis() < 30000L is the first boot so it will try to connect
  // for further attempts it will try with spaces of 60 seconds
  if (WiFi.status() != WL_CONNECTED && (millis() < 30000L || millis() - tLastConnectionAttempt > 60000L)){
    connect();
  }  
}

void loop() {

  connectIfNeeded();

  // Fake value written to EEPROM
  if (epochTime){
    unsigned long now = epochTime + int(millis()/1000);
    short value = random(1, 1000);
    writeReading(now, value);
  }

  blink();
  attendTimers();

  delay(2000);
  
}

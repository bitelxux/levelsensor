// include the library code:
#include <LiquidCrystal.h>

const int GREEN=13;
const int ORANGE=12;
const int RED=11;
const int DELAY=500;

const int EchoPin = 9;
const int TriggerPin = 10;

const int windowSize = 5;
int circularBuffer[windowSize];
int* circularBufferAccessor = circularBuffer;

long sum = 0;
int elementCount = 0;
float mean = 0;

// sound speed constant in centimeters/second
const float VelSon = 34000.0;

char sBuffer[100];

// some tank values

float TANK_RADIUS = 0.6;
float TANK_EMPTY_DISTANCE = 1.08;
float TANK_FULL_DISTANCE= 0.21;
float REMAINING_WATER_HEIGHT= 0.14;

// pre-calculated
float piR2=3.141516*TANK_RADIUS*TANK_RADIUS;

int previous_distance = 0;


// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

int appendToBuffer(int value)
{
  *circularBufferAccessor = value;
  circularBufferAccessor++;
  if (circularBufferAccessor >= circularBuffer + windowSize) 
    circularBufferAccessor = circularBuffer;
}

float addValue(int value)
{
  sum -= *circularBufferAccessor;
  sum += value;
  appendToBuffer(value);
  if (elementCount < windowSize)
    ++elementCount;
  return (float) sum / elementCount;
}

void setup() {
  Serial.begin(115200);
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  pinMode(TriggerPin, OUTPUT);
  pinMode(EchoPin, INPUT);  
}

int ping(int TriggerPin, int EchoPin) {
   long duration, distanceCm;
   
   digitalWrite(TriggerPin, LOW);  // generate a pulse by putting line low for 4us
   delayMicroseconds(4);
   digitalWrite(TriggerPin, HIGH);  // trigger by putting line high for 10us
   delayMicroseconds(10);
   digitalWrite(TriggerPin, LOW);
   
   duration = pulseIn(EchoPin, HIGH);  // how long did it take to go high ? (us)
   
   //distanceCm = duration * 10 / 292/ 2;   // convert distance to centimeters
   distanceCm = duration * 0.000001 * VelSon / 2.0;
   return distanceCm;
}

void printText( unsigned int col, unsigned int row, char* text )
{
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,1);
  lcd.print("                ");  
  lcd.setCursor(col, row);
  lcd.print(text); 
}

void showInfo(unsigned int distance, unsigned int litros) 
{
  sprintf(sBuffer, "d: %d", distance);
  printText(0, 1, sBuffer);
  sprintf(sBuffer, "Litros: %d", litros);
  printText(0, 0, sBuffer);
}

void loop() {
   char s[20];
  
   // mobile average
   float d = addValue(ping(TriggerPin, EchoPin)); // centimeters
   float fd = d/100;

   float h = max(0, TANK_EMPTY_DISTANCE - fd);

   float v = piR2 * h * 1000;

   // There is some remaining water under the floating switch
   // that is not usable because the pumb is off at this level. 
   // Yet, it is water in the tank
   v += piR2 * REMAINING_WATER_HEIGHT * 1000;

   Serial.print("d: ");
   Serial.print(dtostrf(fd, 2, 2, s));
   Serial.print(" h: ");
   Serial.print(dtostrf(h, 2, 2, s));
   Serial.print(" v: ");
   Serial.println(int(v));
 
   showInfo(d, v);
   delay(60000UL);
}

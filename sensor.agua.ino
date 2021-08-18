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

// Constante velocidad sonido en cm/s
const float VelSon = 34000.0;

char sBuffer[100];

// some tank values

float TANK_RADIUS = 0.6;
float TANK_EMPTY_DISTANCE = 108;
float TANK_FULL_DISTANCE= 21;

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
   
   digitalWrite(TriggerPin, LOW);  //para generar un pulso limpio ponemos a LOW 4us
   delayMicroseconds(4);
   digitalWrite(TriggerPin, HIGH);  //generamos Trigger (disparo) de 10us
   delayMicroseconds(10);
   digitalWrite(TriggerPin, LOW);
   
   duration = pulseIn(EchoPin, HIGH);  //medimos el tiempo entre pulsos, en microsegundos
   
   //distanceCm = duration * 10 / 292/ 2;   //convertimos a distancia, en cm
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
   float d = addValue(ping(TriggerPin, EchoPin));
   float fd = d/100.0;
   float h = max(0, (TANK_EMPTY_DISTANCE - d)/100);
   float v = piR2 * h * 1000;

   v += 158; // los 14 cm de agua que quedan en el fondo
   // cuando el flotador apaga el motor


   Serial.print("d: ");
   Serial.print(dtostrf(fd, 2, 2, s));
   Serial.print(" h: ");
   Serial.print(dtostrf(h, 2, 2, s));
   Serial.print(" v: ");
   Serial.println(int(v));
 
   showInfo(d, v);
   delay(60000UL);
}

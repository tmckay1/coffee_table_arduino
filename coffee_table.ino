//ws2812b
#include <FastLED.h>
#define LED_PIN     13
#define NUM_LEDS    96
CRGB wleds[NUM_LEDS];
int ledsPerRow = 12;
CRGB onColor   = CRGB(255, 0, 0);
CRGB offColor  = CRGB(0, 0, 0);
boolean ledsChanged = false;

// multiplexers
const int firstMultiplexerPins[3]  = {51, 49, 47}; // S0~2, S1~3, S2~4 multiplex1 (first huge chain of 8 multiplexers)
const int secondMultiplexerPins[3] = {35, 33, 29}; // S0~2, S1~3, S2~4 multiplex2 (second smaller chain of 3 multiplexers)
const int readMultiplexerPins[3]   = {43, 41, 39}; // S0~2, S1~3, S2~4 multiplex3 (multiplexer to read from)
const int multiplexerInput1        = A0;           // First  input to read from 8 multiplexers
const int multiplexerInput2        = A1;           // Second input to read from 3 multiplexers

// emitters
int first74LatchPin  = 4; // Latch pin of first  74HC595 that controls 8 columns
int first74ClockPin  = 3; // Clock pin of first  74HC595 that controls 8 columns
int first74DataPin   = 2; // Data  pin of first  74HC595 that controls 8 columns
int second74LatchPin = 7; // Latch pin of second 74HC595 that controls 3 columns
int second74ClockPin = 6; // Clock pin of second 74HC595 that controls 3 columns
int second74DataPin  = 5; // Data  pin of second 74HC595 that controls 3 columns
byte leds = 0;            // Variable to hold the pattern of which LEDs are currently turned on or off. In our case, we want them all on at once

// photodiodes
const int totalSensors = 96;
const int threshold = 150;
int baseVal[totalSensors];
boolean isOn[totalSensors];

int ledIndexes[66][4] = {
  {0, 1, 22, 23}, // 0
  {1, 2, 21, 22},
  {2, 3, 20, 21},
  {3, 4, 19, 20},
  {4, 5, 18, 19},
  {5, 6, 17, 18}, // 5
  {6, 7, 16, 17},
  {7, 8, 15, 16},
  {8, 9, 14, 15},
  {9, 10, 13, 14},
  {10, 11, 12, 13}, // 10
  {22, 23, 24, 25},
  {21, 22, 25, 26},
  {20, 21, 26, 27},
  {19, 20, 27, 28},
  {18, 19, 28, 29}, // 15
  {17, 18, 29, 30},
  {16, 17, 30, 31},
  {15, 16, 31, 32},
  {14, 15, 32, 33},
  {13, 14, 33, 34}, // 20
  {12, 13, 34, 35},
  {24, 25, 46, 47},
  {25, 26, 45, 46},
  {26, 27, 44, 45},
  {27, 28, 43, 44}, // 25
  {28, 29, 42, 43},
  {29, 30, 41, 42},
  {30, 31, 40, 41},
  {31, 32, 39, 40},
  {32, 33, 38, 39}, // 30
  {33, 34, 37, 38},
  {34, 35, 36, 37},
  {46, 47, 48, 49},
  {45, 46, 49, 50},
  {44, 45, 50, 51}, // 35
  {43, 44, 51, 52},
  {42, 43, 52, 53},
  {41, 42, 53, 54},
  {40, 41, 54, 55},
  {39, 40, 55, 56}, // 40
  {38, 39, 56, 57},
  {37, 38, 57, 58},
  {36, 37, 58, 59},
  {},
  {}, // 45
  {},
  {},
  {},
  {},
  {}, // 50
  {},
  {},
  {},
  {},
  {48, 49, 70, 71}, // 55
  {49, 50, 69, 70},
  {50, 51, 68, 69},
  {51, 52, 67, 68},
  {52, 53, 66, 67},
  {53, 54, 65, 66}, // 60
  {54, 55, 64, 65},
  {55, 56, 63, 64},
  {56, 57, 62, 63},
  {57, 58, 61, 62},
  {58, 59, 60, 61}, // 65
};

void setup() 
{
  Serial.begin(9600); // Initialize the serial port

  // Set up the multiplexers as outputs
  for (int i=0; i<3; i++)
  {
    pinMode(firstMultiplexerPins[i], OUTPUT);
    pinMode(secondMultiplexerPins[i], OUTPUT);
    pinMode(readMultiplexerPins[i], OUTPUT);
    digitalWrite(firstMultiplexerPins[i], LOW);
    digitalWrite(secondMultiplexerPins[i], LOW);
    digitalWrite(readMultiplexerPins[i], LOW);
  }
  
  // Turn on emitters
  pinMode(first74LatchPin, OUTPUT);
  pinMode(first74ClockPin, OUTPUT);  
  pinMode(first74DataPin, OUTPUT);
  pinMode(second74LatchPin, OUTPUT);
  pinMode(second74ClockPin, OUTPUT);  
  pinMode(second74DataPin, OUTPUT);
  bitSet(leds, 7);
  digitalWrite(first74LatchPin, LOW);
  digitalWrite(first74LatchPin, LOW);
  shiftOut(first74DataPin, first74ClockPin, LSBFIRST, leds);
  shiftOut(second74DataPin, second74ClockPin, LSBFIRST, leds);
  digitalWrite(second74LatchPin, HIGH);
  digitalWrite(second74LatchPin, HIGH);

  // setup photodiode array
  for (int i=0; i<totalSensors; i++)
  {
    baseVal[i] = -1;
  }

  // setup ws2812b leds
  FastLED.addLeds<WS2812, LED_PIN, GRB>(wleds, NUM_LEDS);
}

void loop() 
{
  resetLEDs();
//  readInput1(8, multiplexerInput1, firstMultiplexerPins, 0);
  readMultiplexerChain(8, multiplexerInput1, firstMultiplexerPins, 0);
  readMultiplexerChain(4, multiplexerInput2, secondMultiplexerPins, 8); // add offset since we start the second chain on the 9th column
  // setup photodiode array
//  for (int i=0; i<totalSensors; i++)
//  {
//    Serial.print("index: ");
//    Serial.println(i);
//    Serial.print("baseVal[ledIndex]: ");
//    Serial.println(baseVal[i]);
//    Serial.println("");
//  }
  if(ledsChanged) {
    FastLED.show();
  }
}

void readInput1(int numMultiplexers, int input, int multiplexerPins[], int offset)
{
  int multiplexerIndex = 1;

  // set the pins to read from the current multiplexer in the chain
  selectPinForMultiplexer(multiplexerIndex, readMultiplexerPins);

  // for this multiplexer, read the 7 photodiode inputs in the column
  int pin = 0;
  
  // set the pins to read from the current photodiode
  selectPinForMultiplexer(pin, multiplexerPins);
  
  // turn on leds
  int photoDiodeIndex = multiplexerIndex + offset + pin * 11;
  setLEDs(input, photoDiodeIndex, multiplexerIndex, offset, pin);
}

void readMultiplexerChain(int numMultiplexers, int input, int multiplexerPins[], int offset)
{
  // Loop through the chain of multiplexers
  for (byte multiplexerIndex = 0; multiplexerIndex < numMultiplexers; multiplexerIndex++)
  {
    // set the pins to read from the current multiplexer in the chain
    selectPinForMultiplexer(multiplexerIndex, readMultiplexerPins);

    // for this multiplexer, read the 7 photodiode inputs in the column
    for (int pin = 0; pin < 7; pin++)
    {
      // set the pins to read from the current photodiode
      selectPinForMultiplexer(pin, multiplexerPins);
      
      // turn on leds
      int photoDiodeIndex = multiplexerIndex + offset + pin * 11;
      setLEDs(input, photoDiodeIndex, multiplexerIndex, offset, pin);
    }
  }
}

void selectPinForMultiplexer(byte index, int multiplexerPins[])
{
  for (int i=0; i<3; i++) {
    digitalWrite(multiplexerPins[i], index & (1<<i)?HIGH:LOW);
  }
}

void resetLEDs()
{
  ledsChanged = false;
  for(int i = 0; i < NUM_LEDS; i++) {
    wleds[i] = offColor;
  }
}

void setLEDs(int input, int photoDiodeIndex, int multiplexerIndex, int offset, int pin)
{
  int inputValue = analogRead(input);
  if(baseVal[photoDiodeIndex] < 0) {
    baseVal[photoDiodeIndex] = inputValue;
  }
  
  if(inputValue - baseVal[photoDiodeIndex] > threshold){
    wleds[ledIndexes[photoDiodeIndex][0]] = onColor;
    wleds[ledIndexes[photoDiodeIndex][1]] = onColor;
    wleds[ledIndexes[photoDiodeIndex][2]] = onColor;
    wleds[ledIndexes[photoDiodeIndex][3]] = onColor;
    if(!isOn[photoDiodeIndex]) {
      ledsChanged = true;
    }
    isOn[photoDiodeIndex] = true;
    Serial.print("ledIndexes[photoDiodeIndex]: ");
    Serial.print(ledIndexes[photoDiodeIndex][0]);
    Serial.print(", ");
    Serial.print(ledIndexes[photoDiodeIndex][1]);
    Serial.print(", ");
    Serial.print(ledIndexes[photoDiodeIndex][2]);
    Serial.print(", ");
    Serial.println(ledIndexes[photoDiodeIndex][3]);
    Serial.print("photoDiodeIndex: ");
    Serial.println(photoDiodeIndex);
    Serial.print("inputValue: ");
    Serial.println(inputValue);
    Serial.print("threshold: ");
    Serial.println(threshold);
    Serial.print("baseVal[photoDiodeIndex]: ");
    Serial.println(baseVal[photoDiodeIndex]);
    Serial.print("multiplexerIndex: ");
    Serial.println(multiplexerIndex);
    Serial.print("offset: ");
    Serial.println(offset);
    Serial.print("pin: ");
    Serial.println(pin);
    Serial.println("");
  }else{
    if(isOn[photoDiodeIndex]) {
      ledsChanged = true;
    }
    isOn[photoDiodeIndex] = false;
  }
}

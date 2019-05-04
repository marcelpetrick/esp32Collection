#include "SPI.h"
#include "LedMatrix.h"

// defines
#define NUMBER_OF_DEVICES 1 //number of led matrix connected serially
#define CS_PIN 15
#define CLK_PIN 14
#define MISO_PIN 2 //we do not use this pin - just fill to match constructor
#define MOSI_PIN 12

// button defines
int bootButtonPin{0};
int bigDomePin{17};

// global stuff
LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);

void setup()
{
  ledMatrix.init();
  ledMatrix.setIntensity(4); // range is 0-15
  ledMatrix.setText("The quick brown fox jumps over the lazy dog");
}

void loop()
{
  ledMatrix.clear();
  ledMatrix.scrollTextLeft();
  ledMatrix.drawText();
  ledMatrix.commit();
  delay(200);
}

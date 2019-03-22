// randomReviwer: smol fun project to relieve the stress of finding a fitting reviewer
//
// after booting:
// if button "boot" clicked (GPIO17) -> then select randomly one of the entries from the list of possible reviewers

// includes
#include "SPI.h"
#include "WiFi.h"
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

//----------------------------------------------------------------------------------------------------

void setup()
{
  pinMode(bootButtonPin, INPUT);
  pinMode(bigDomePin, INPUT);

  Serial.begin(115200);

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  ledMatrix.init();

  Serial.println("*** Setup done ***");
}

//----------------------------------------------------------------------------------------------------

bool checkButtons()
{
  //Serial.println("checkButtons()");

  bool returnValue(false);

  // after booting the GPIO0 for BOOT is free for use
  //  if (digitalRead(bootButtonPin) == HIGH)
  //  {
  //    Serial.println("bootButtonPin pressed");
  //    returnValue |= true;
  //  }

  if (digitalRead(bigDomePin) == HIGH)
  {
    Serial.println("bigDomePin pressed");
    returnValue |= true;
  }

  return returnValue;
}

//----------------------------------------------------------------------------------------------------

#define PEOPLEARRAYSIZE 8
// @brief function to determine randomly inside a given array one of the values
// @returns String like "MPE was picked".
String getRandomReviewer()
{
  Serial.println("getRandomReviewer()");

  // init the array of possible reviewers
  String people[PEOPLEARRAYSIZE] = {
    "GSC",
    "MSA",
    "MPE",
    "NLE",
    "RNI",
    "HGA",
    "MDR",
    "NKU",
  };

  // randomly determine an index
  int const randomIndex = random(PEOPLEARRAYSIZE); // exclusive the last value
  Serial.println(randomIndex);

  // create the result-string
  String returnValue = people[randomIndex] + " is your guy! :)";

  // and return it
  return returnValue;
}

//----------------------------------------------------------------------------------------------------

void loop()
{
  ledMatrix.clear();
  bool const buttonWasPressed = checkButtons();
  if (buttonWasPressed)
  {
    String const resultString = getRandomReviewer();
    Serial.println(resultString);

    ledMatrix.setText(resultString);
    for (int painter = 0; painter < 333; painter++)
    {
      ledMatrix.clear();
      ledMatrix.scrollTextLeft();
      ledMatrix.drawText();
      ledMatrix.commit();
      delay(50);

//      // add some early abort
//      if (checkButtons())
//      {
//        Serial.println("skip now the scrolling!");
//        break;
//      }
    }

    ledMatrix.clear();
  }
}


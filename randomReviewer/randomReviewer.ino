// after booting:
// scan networks and print them to the matrix
// if button "boot" clicked (GPIO0?), then select randomly one of the entries from the list of possible reviewers

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

int inPin = 0;

// global stuff
LedMatrix ledMatrix = LedMatrix(NUMBER_OF_DEVICES, CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
String output;

//----------------------------------------------------------------------------------------------------

void setup()
{
  pinMode(inPin, INPUT);

  Serial.begin(115200);

  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("Setup done");

  ledMatrix.init();
}

//----------------------------------------------------------------------------------------------------

bool checkBootButton()
{
  Serial.println("checkBootButton()");

  bool returnValue(false);

  // after booting the GPIO0 for BOOT is free for use
  if (digitalRead(inPin) == HIGH)
  {
    //Serial.println("--if :)");
    returnValue = true;
  }
  else
  {
    //Serial.println("--else");
    // dot nothing
  }

  return !returnValue;
}

//----------------------------------------------------------------------------------------------------

#define PEOPLEARRAYSIZE 9
// @brief function to determine randomly inside a given array one of the values
// @returns String like "MP was picked".
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
    "MLA",
    "HGA", //7
    "MDR",
    "NKU", //9
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
  Serial.println("###### loop() ######");
  bool const gpio0pressed = checkBootButton();
  if (gpio0pressed)
  {
    String resultString = getRandomReviewer();
    Serial.println(resultString);

    ledMatrix.setText(resultString);
    for (int painter = 0; painter < 265; painter++)
    {
      ledMatrix.clear();
      ledMatrix.scrollTextLeft();
      ledMatrix.drawText();
      ledMatrix.commit();
      delay(50);

      // add some early abort
      if (checkBootButton())
      {
          Serial.println("skip now the scrolling!");
          break;
      }
    }
    ledMatrix.clear();
  }

  Serial.println("scan start");

  // WiFi.scanNetworks will return the number of networks found
  int const n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
  {
    Serial.println("no networks found");
    output += "no networks";
  }
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    output += n;
    output += " networks";

    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      //delay(10);
    }
  }
  Serial.println("");

  Serial.println(output);
  ledMatrix.setText(output);
  for (int painter = 0; painter < 180; painter++)
  {
    if (checkBootButton())
    {
      Serial.println("skip now the scrolling!");
      break;
    }

    ledMatrix.clear();
    ledMatrix.scrollTextLeft();
    ledMatrix.drawText();
    ledMatrix.commit();
    delay(50);
  }
  ledMatrix.clear();

  output = "";
}


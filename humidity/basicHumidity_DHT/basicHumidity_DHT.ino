#include "DHT.h"

// use G16 of ESP32 to read data
const int DHTPin = 16;

//our sensor is DHT11 type
#define DHTTYPE DHT11

//create an instance of DHT sensor
DHT dht(DHTPin, DHTTYPE);

void setup()
{
  Serial.begin(115200);
  Serial.println("DHT11 sensor!");
  //call begin to start sensor
  dht.begin();
}

void loop() {
  //use the functions which are supplied by library.
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(500); // wait a bit
    return;
  }
  
  // print the result to Terminal
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" °C ");
  
  //we delay a little bit for next read
  delay(2000);
}


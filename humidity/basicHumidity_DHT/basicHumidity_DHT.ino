#include "DHT.h"

// use G23 of ESP32 to read data
const int DHTPin = 23;

//our sensor is DHT11 type
#define DHTTYPE DHT22

//create an instance of DHT sensor
DHT dht(DHTPin, DHTTYPE);

void setup()
{
  Serial.begin(115200);
  Serial.println("DHT22 sensor!");
  //call begin to start sensor
  dht.begin();
}

void loop() {
  //use the functions which are supplied by library.
  double const humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  double const temperature = dht.readTemperature();
  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    delay(1000); // wait a bit
    return;
  }

  // print the result to Terminal
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" °C ");

  // delay a bit for the next read
  delay(2000);
}


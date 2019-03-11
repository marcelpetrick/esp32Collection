#include "DHT.h"

#ifdef __cplusplus
extern "C" {
#endif
uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

// use G23 of ESP32 to read data
const int DHTPin = 23;
const int YL69Pin = 34;

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
  // *** DHT22 measurement ***
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
  Serial.print("Humidity (DHT22): ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Temperature (DHT22): ");
  Serial.print(temperature);
  Serial.println(" °C ");

  // *** internal temperature ***
  //convert raw temperature in F to Celsius degrees
  Serial.print("Temperature (internal): ");
  Serial.print((temprature_sens_read() - 32) / 1.8);
  Serial.println(" °C");

  // ** YL-69 moisture ***
  int const readYL69value = analogRead(YL69Pin);
  // map inversely to 0..10%
  int const convertedPercentage = map(readYL69value, 4095, 1200, 0, 100);
  Serial.print("Moisture (YL-69): ");
  Serial.print(convertedPercentage);
  Serial.print("%\n");

  // delay a bit for the next read
  delay(2000);
}


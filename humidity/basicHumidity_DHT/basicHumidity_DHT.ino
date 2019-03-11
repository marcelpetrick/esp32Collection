#include "DHT.h"

// for BME: followed mostly https://randomnerdtutorials.com/esp32-web-server-with-bme280-mini-weather-station/
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C
// end of for BME

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

//our sensor is DHT11 type
#define DHTTYPE DHT22

//create an instance of DHT sensor
DHT dht(DHTPin, DHTTYPE);

void setup()
{
  Serial.begin(115200);
  //Serial.begin(9600);
  
  Serial.println("DHT22 sensor!");
  //call begin to start sensor
  dht.begin();

  // for the BME
  Serial.println(F("BME280 test"));

  bool status;

  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = bme.begin(0x77);
  if (!status)
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  Serial.println();
}

void loop() {
  Serial.println("----- loop() -----");
  //use the functions which are supplied by library.
  double const humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  double const temperature = dht.readTemperature();
  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature))
  {
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

  // Convert raw temperature in F to Celsius degrees
  Serial.print("Temperature (internal): ");
  Serial.print((temprature_sens_read() - 32) / 1.8);
  Serial.println(" °C");

  printValues();

  // delay a bit for the next read
  delay(2000);
}

void printValues()
{
  Serial.println("----- printValues() -----");
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println(" °C");

  Serial.print("Pressure = ");

  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");

  Serial.println();
}

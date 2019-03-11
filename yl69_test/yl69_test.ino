int sensor_pin = 34;
int output_value ;
void setup()
{
  Serial.begin(115200);
  Serial.println("Reading From the Sensor ...");
  delay(2000);
}

void loop()
{
  output_value = analogRead(sensor_pin);
  Serial.print("Moisture : ");
  Serial.print(output_value);
  Serial.print("\n");
  delay(1000);
}

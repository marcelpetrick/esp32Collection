#include <Arduino.h>
#include <U8x8lib.h>

// Pin 16   OLED Reset
// Pin 4,5  I2C

// use Board "Wemos D1 Mini R2", 80Mhz, 4 MB Flash, 921600baud to flash
// COM10

U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(16);

void setup(void)
{
  u8x8.begin();
}

void loop(void)
{
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 0, "Laku noc,");
  delay(1000);
  u8x8.clearLine(0);
  u8x8.drawString(0, 2, "Puska! <3");
  delay(1000);
  u8x8.clearLine(2);
}

// question
// unit under test is a: ESP8266 NodeMCU v3.2 from LoLin
// is Pin 2 the correct one for the LED?
int pin = 2;

void setup() {
  // initialize GPIO 2 as an output.
  pinMode(pin, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(pin, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(333);               // wait for a second
  digitalWrite(pin, LOW);    // turn the LED off by making the voltage LOW
  delay(333);               // wait for a second
}

// ### used settings ###
// Build options changed, rebuilding all
// Archiving built core (caching) in: C:\Users\TANTEE~1\AppData\Local\Temp\arduino_cache_529265\core\core_esp8266_esp8266_generic_xtal_80,vt_flash,exception_disabled,ResetMethod_nodemcu,CrystalFreq_26,FlashFreq_40,FlashMode_dio,eesz_512K64,led_2,ip_lm2f,dbg_Disabled,lvl_None____,wipe_none,baud_115200_1d41b34226a4629aaa13e53689369efa.a
// Sketch uses 258624 bytes (59%) of program storage space. Maximum is 434160 bytes.
// Global variables use 26700 bytes (32%) of dynamic memory, leaving 55220 bytes for local variables. Maximum is 81920 bytes.
// Uploading 262768 bytes from C:\Users\TANTEE~1\AppData\Local\Temp\arduino_build_73624/basicBlink_nodemcuv32_esp8266.ino.ino.bin to flash at 0x00000000
// ................................................................................ [ 31% ]
// ................................................................................ [ 62% ]
// ................................................................................ [ 93% ]
// .................                                                                [ 100% ]


#include <Arduino.h>
#include "ArduinoMessage.h"
#include <WifiManager.h>

const int led = LED_BUILTIN;

WiFiManager wm;

HardwareSerial SerialJlink(1);

// #define SerialDebug SerialJlink
// #define SerialComm Serial
#define SerialDebug Serial
#define SerialComm SerialJlink

ArduinoMessage messager(&SerialComm, &SerialDebug);

/*
Change in Wire.h
#define I2C_BUFFER_LENGTH 256 
*/

void setup()
{
  SerialDebug.begin(115200);
  SerialComm.setRxBufferSize(4096);
  SerialComm.begin(921600, SERIAL_8N1, 27, 26);
  pinMode(led, OUTPUT);

  // explicitly set mode, esp defaults to STA+AP
  WiFi.mode(WIFI_STA);
  //reset settings - wipe credentials for testing
  //wm.resetSettings();
  wm.setConnectTimeout(10);
  wm.setConfigPortalBlocking(false);
  //automatically connect using saved credentials if they exist
  //If connection fails it starts an access point with the specified name
  if (wm.autoConnect("ESP32Processor", "88888888"))
  {
    Serial.println("connected...yeey :)");
  }
  else
  {
    Serial.println("Configportal running");
  }
}

uint32_t last = millis();
int n_avail = 0;
uint8_t temp_buffer[32];
uint8_t led_status = 1;
void loop()
{
  wm.process();
  while ((n_avail = SerialComm.available()) > 0)
  {
    if (n_avail > sizeof(temp_buffer))
    {
      n_avail = sizeof(temp_buffer);
    }
    int n_read = SerialComm.readBytes((char *)temp_buffer, n_avail);
    for (int i = 0; i < n_read; i++)
    {
      messager.message_decoder_fill_data(temp_buffer[i]);
    }
  }
  // delay(1);
  if (millis() > last + 500)
  {
    last = millis();
    digitalWrite(led, led_status);
    led_status = 1 - led_status;
    // SerialDebug.println(millis());
    // SerialComm.println(millis());
  }
}

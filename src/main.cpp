#include <Arduino.h>
#include "ArduinoMessage.h"

const int led = LED_BUILTIN;

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
}

uint32_t last = millis();
int n_avail = 0;
uint8_t temp_buffer[32];
uint8_t led_status = 1;
void loop()
{
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

#include <Arduino.h>
#include "ArduinoMessage.h"

const int led = LED_BUILTIN;

/*

In order to make max raw(decoded) message == 250:

cdc_queue.h:
#define CDC_TRANSMIT_QUEUE_BUFFER_SIZE ((uint16_t)(CDC_QUEUE_MAX_PACKET_SIZE * 8))
#define CDC_RECEIVE_QUEUE_BUFFER_SIZE ((uint16_t)(CDC_QUEUE_MAX_PACKET_SIZE * 8))

*/

HardwareSerial SerialJlink(PB11, PB10);

#define SerialDebug SerialJlink
#define SerialComm Serial

ArduinoMessage messager(&SerialComm, &SerialDebug);

void setup()
{
  SerialDebug.begin(115200);
  pinMode(led, OUTPUT);

  SerialComm.begin(115200);
}

uint32_t last = millis();
void loop()
{
  while (SerialComm.available())
  {
    int c = SerialComm.read();
    if (c < 0)
    {
      SerialDebug.printf("Serial1 read error: %d\n", c);
    }
    else
    {
      messager.message_decoder_fill_data(c);
    }
  }
  delay(1);
  if (millis() > last + 500)
  {
    last = millis();
    digitalToggle(led);
  }
}

#include <Arduino.h>
#include "ArduinoMessage.h"

// const int led = LED_BUILTIN;

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
  // pinMode(led, OUTPUT);

  SerialComm.begin(115200);
}

uint32_t last = millis();
int n_avail = 0;
uint8_t temp_buffer[32];
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
  // if (millis() > last + 500)
  // {
  //   last = millis();
  //   // digitalToggle(led);
  // }
}

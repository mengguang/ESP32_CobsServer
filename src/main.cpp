#include <Arduino.h>
#include "CobsMessage.h"
#include "ArduinoCommand.h"
#include <Wire.h>

const int led = LED_BUILTIN;
const int led2 = PA0;

HardwareSerial SerialJlink(PB11, PB10);

#define SerialDebug SerialJlink
#define SerialComm Serial

int command_processor(const uint8_t *message, uint32_t message_length, uint8_t *result, uint32_t result_length)
{
  uint8_t cmd = message[0];
  int reply_length = 0;
  switch (cmd)
  {
  case CMD_PIN_MODE:
  {
    pinMode(message[1], message[2]);
    result[0] = 0x01;
    reply_length = CMD_PIN_MODE_RL;
    break;
  }
  case CMD_DIGITAL_WRITE:
  {
    digitalWrite(message[1], message[2]);
    result[0] = 0x01;
    reply_length = CMD_DIGITAL_WRITE_RL;
    break;
  }
  case CMD_DIGITAL_READ:
  {
    int value = digitalRead(message[1]);
    result[0] = 0x01;
    result[1] = (uint8_t)value;
    reply_length = CMD_DIGITAL_READ_RL;
    break;
  }
  case CMD_WIRE_BEGIN:
  {
    uint8_t sda = message[1];
    uint8_t scl = message[2];
    Wire.begin(sda, scl);
    result[0] = 0x01;
    reply_length = CMD_WIRE_BEGIN_RL;
    break;
  }
  case CMD_WIRE_REQUEST_FROM:
  {
    uint8_t address = message[1];
    uint8_t n_read = message[2];
    uint8_t ret = Wire.requestFrom(address, n_read);
    result[0] = 0x01;
    result[1] = ret; //read back length
    if (ret + 2 > result_length)
    {
      //result overflow, do not return any data.
      ret = 0;
    }
    if (ret > 0)
    {

      Wire.readBytes(result + 2, ret);
    }
    reply_length = 2 + ret;
    break;
  }
  case CMD_WIRE_TRANSMISSION:
  {
    uint8_t address = message[1];
    uint32_t data_length = message_length - 2;
    Wire.beginTransmission(address);
    if (data_length > 0)
    {
      Wire.write(message + 2, data_length);
    }
    uint8_t ret = Wire.endTransmission();
    result[0] = 0x01;
    result[1] = ret;
    reply_length = CMD_WIRE_TRANSMISSION_RL;
    break;
  }

  default:
    break;
  }
  return reply_length;
}

void hex_dump(const uint8_t *data, uint32_t length)
{
  for (uint32_t i = 0; i < length; i++)
  {
    SerialDebug.printf("%02X ", data[i]);
    if ((i + 1) % 16 == 0)
    {
      SerialDebug.printf("\n");
    }
  }
  if (length % 16 != 0)
  {
    SerialDebug.printf("\n");
  }
}

class SimpleCobsMessage : public CobsMessage
{
public:
  void debug_message_writer(const char *message)
  {
    SerialDebug.print(message);
  }
  bool message_send_writer(const uint8_t *data, uint32_t length)
  {
    // SerialDebug.print("send back encoded data:\n");
    // hex_dump(data, length);
    int n = SerialComm.write(data, length);
    // SerialDebug.printf("write back size: %d\n", n);
    return true;
  }
  bool message_processor(const uint8_t *data, uint32_t length)
  {
    SerialDebug.print("request: \n");
    hex_dump(data, length);

    //reuse receive buffer
    //+ ENCODE_OFFSET enable in-place encoding
    uint8_t *reply_buffer = received_message_buffer + ENCODE_OFFSET;
    int reply_length = command_processor(data, length, reply_buffer, MAX_DECODED_MESSAGE_SIZE);
    if (reply_length <= 0)
    {
      debug_printf("command processor error: %d\n", reply_length);
      return false;
    }
    SerialDebug.print("reply: \n");
    hex_dump(reply_buffer, reply_length);
    return send_message(reply_buffer, reply_length);
  }
};

SimpleCobsMessage messager;

void setup()
{
  SerialDebug.begin(115200);
  pinMode(led, OUTPUT);

  SerialComm.begin(115200);
}

uint32_t last = millis();
void loop()
{
  if (SerialComm.available())
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
    delay(1);
  }
  if (millis() > last + 500)
  {
    last = millis();
    digitalToggle(led);
  }
  delay(1);
}

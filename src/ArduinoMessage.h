#include "ArduinoCommand.h"
#include "CobsMessage.h"
#include "Arduino.h"
#include "Wire.h"
#include "SPI.h"

class InterruptHandler
{
public:
    uint8_t pin;
    bool attached = false;
    bool triggered = false;

    void handler()
    {
        triggered = true;
    }

    bool has_triggered()
    {
        return attached && triggered;
    }

private:
};

class ArduinoMessage : public CobsMessage
{
private:
    Stream *serialComm = nullptr;
    Stream *serialDebug = nullptr;
    bool debug = false;
    InterruptHandler interruprHandler[MAX_INTERRUPT_NUMBER];

public:
    ArduinoMessage(Stream *_serialComm, Stream *_serialDebug = nullptr)
    {
        serialComm = _serialComm;
        serialDebug = _serialDebug;
    }

    void set_debug(bool _debug)
    {
        debug = _debug;
    }

    void debug_message_writer(const char *message)
    {
        if (debug && serialDebug)
        {
            serialDebug->print(message);
        }
    }
    bool message_send_writer(const uint8_t *data, uint32_t length)
    {
        // debug_printf("send back encoded data:\n");
        // hex_dump(data, length);
        int n = serialComm->write(data, length);
        return true;
    }
    bool message_processor(const uint8_t *data, uint32_t length)
    {
        debug_printf("request: \n");
        hex_dump(data, length);

        //reuse receive buffer
        //+ ENCODE_OFFSET enable in-place encoding
        uint8_t *reply_buffer = received_message_buffer + ENCODE_OFFSET;
        int reply_length = command_processor(data, length, reply_buffer, MAX_DECODED_MESSAGE_SIZE);
        bool ret = false;
        if (reply_length < 0)
        {
            debug_printf("command processor error: %d\n", reply_length);
            ret = false;
        }
        else if (reply_length > 0)
        {
            debug_printf("reply: \n");
            hex_dump(reply_buffer, reply_length);
            ret = send_message(reply_buffer, reply_length);
            check_and_send_interrupt_message();
        }
        else
        { //no need reply
            ret = true;
        }

        return ret;
    }
    bool check_and_send_interrupt_message()
    {
        uint8_t result[EVENT_MESSAGE_SIZE] = {0};
        result[0] = EVENT_DIGITAL_INTERRUPT;
        bool need_send = false;
        for (int i = 0; i < MAX_INTERRUPT_NUMBER; i++)
        {
            if (interruprHandler[i].has_triggered())
            {
                need_send = true;
                result[1 + i] = 1;
                interruprHandler[i].triggered = false;
            }
        }
        if (need_send)
        {
            send_message(result, sizeof(result));
        }
        return true;
    }
    void hex_dump(const uint8_t *data, uint32_t length)
    {
        debug_printf("length: %u\n", length);
        if (length > 16)
        {
            length = 16;
        }
        for (uint32_t i = 0; i < length; i++)
        {
            debug_printf("%02X ", data[i]);
            if ((i + 1) % 16 == 0)
            {
                debug_printf("\n");
            }
        }
        if (length % 16 != 0)
        {
            debug_printf("\n");
        }
    }
    int command_processor(const uint8_t *message, uint32_t message_length, uint8_t *result, uint32_t result_length)
    {
        uint8_t cmd = message[0];
        int reply_length = 0;
        switch (cmd)
        {
        case CMD_SYS_SET_DEBUG:
        {
            bool _debug = (message[1] != 0);
            set_debug(_debug);
            result[0] = 0x01;
            reply_length = CMD_SYS_SET_DEBUG_RL;
            break;
        }
        case CMD_PIN_MODE:
        {
            pinMode(message[1], message[2]);
            result[0] = 0x01;
            reply_length = CMD_PIN_MODE_RL;
            break;
        }
        case CMD_DIGITAL_WRITE:
        {
            uint8_t pin = message[1];
            uint8_t value = message[2];
            bool need_reply = message[3];
            digitalWrite(pin, value);
            result[0] = 0x01;
            reply_length = CMD_DIGITAL_WRITE_RL;
            if (!need_reply)
            {
                reply_length = 0;
            }
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

        case CMD_DIGITAL_ATTACH_INTERRUPT:
        {
            uint8_t id = message[1];
            uint8_t pin = message[2];
            uint8_t mode = message[3];
            if (id >= MAX_INTERRUPT_NUMBER)
            {
                result[0] = 0x00;
                reply_length = CMD_DIGITAL_ATTACH_INTERRUPT_RL;
            }
            else
            {
                if (interruprHandler[id].attached)
                {
                    debug_printf("pin %d has attached, detach it.\n", interruprHandler[id].pin);
                    detachInterrupt(interruprHandler[id].pin);
                }
                interruprHandler[id].attached = true;
                interruprHandler[id].pin = pin;
                interruprHandler[id].triggered = false;
                callback_function_t callback = std::bind(&InterruptHandler::handler, &(interruprHandler[id]));
                attachInterrupt(pin, callback, mode);
                result[0] = 0x01;
                reply_length = CMD_DIGITAL_ATTACH_INTERRUPT_RL;
            }
            break;
        }
        case CMD_DIGITAL_DETACH_INTERRUPT:
        {
            uint8_t id = message[1];
            if (id >= MAX_INTERRUPT_NUMBER)
            {
                result[0] = 0x00;
                reply_length = CMD_DIGITAL_DETACH_INTERRUPT_RL;
            }
            else
            {
                detachInterrupt(interruprHandler[id].pin);
                interruprHandler[id].attached = false;
                interruprHandler[id].triggered = false;
                result[0] = 0x01;
                reply_length = CMD_DIGITAL_DETACH_INTERRUPT_RL;
            }
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
                result[0] = 0x00;
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

        case CMD_SPI_BEGIN:
        {
            uint8_t mosi = message[1];
            uint8_t miso = message[2];
            uint8_t sclk = message[3];
            SPI.setMOSI(mosi);
            SPI.setMISO(miso);
            SPI.setSCLK(sclk);
            SPI.begin();
            result[0] = 0x01;
            reply_length = CMD_SPI_BEGIN_RL;
            break;
        }
        case CMD_SPI_END:
        {
            SPI.end();
            result[0] = 0x01;
            reply_length = CMD_SPI_END_RL;
            break;
        }
        case CMD_SPI_BEGIN_TRANSACTION:
        {
            //4 bytes clock(MSB), 1 byte bitOrder, 1 byte dataMode
            uint32_t clock = 0;
            clock += message[1] << 24;
            clock += message[2] << 16;
            clock += message[3] << 8;
            clock += message[4];
            BitOrder bitOrder = message[5] == 0 ? LSBFIRST : MSBFIRST;
            uint8_t dataMode = message[6];
            auto s = SPISettings(clock, bitOrder, dataMode);
            SPI.beginTransaction(s);
            result[0] = 0x01;
            reply_length = CMD_SPI_BEGIN_TRANSACTION_RL;
            break;
        }
        case CMD_SPI_END_TRANSACTION:
        {
            SPI.endTransaction();
            result[0] = 0x01;
            reply_length = CMD_SPI_END_TRANSACTION_RL;
            break;
        }
        case CMD_SPI_TRANSFER:
        {
            uint32_t data_length = message_length - 1;
            if (data_length > 0)
            {
                uint8_t spi_buffer[MAX_ENCODED_MESSAGE_SIZE] = {0};
                memcpy(spi_buffer, message + 1, data_length);
                SPI.transfer(spi_buffer, data_length);
                result[0] = 0x01;
                memcpy(result + 1, spi_buffer, data_length);
                reply_length = 1 + data_length;
            }
            else
            {
                result[0] = 0x00;
                reply_length = 1 + data_length;
            }
            break;
        }
        case CMD_SPI_WRITE:
        {
            bool need_reply = message[1];
            uint8_t data_length = message_length - 2;
            if (data_length > 0)
            {
                uint8_t spi_buffer[MAX_ENCODED_MESSAGE_SIZE] = {0};
                memcpy(spi_buffer, message + 2, data_length);
                SPI.transfer(spi_buffer, data_length);
                result[0] = 0x01;
                reply_length = CMD_SPI_WRITE_RL;
            }
            else
            {
                result[0] = 0x00;
                reply_length = CMD_SPI_WRITE_RL;
            }
            if (!need_reply)
            {
                reply_length = 0;
            }
            break;
        }
        case CMD_SPI_READ:
        {
            uint8_t data_length = message[1];
            if (data_length > 0)
            {
                uint8_t spi_buffer[MAX_ENCODED_MESSAGE_SIZE] = {0};
                SPI.transfer(spi_buffer, data_length);
                result[0] = 0x01;
                memcpy(result + 1, spi_buffer, data_length);
                reply_length = 1 + data_length;
            }
            else
            {
                result[0] = 0x00;
                reply_length = 1 + data_length;
            }
            break;
        }

        default:
            break;
        }
        return reply_length;
    }
};

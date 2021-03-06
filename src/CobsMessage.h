#pragma once

#include "cobs.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>

#define MAX_DECODED_MESSAGE_SIZE 250
#define MAX_ENCODED_MESSAGE_SIZE COBS_ENCODE_DST_BUF_LEN_MAX(MAX_DECODED_MESSAGE_SIZE)
#define RECEIVE_BUFFER_SIZE (MAX_ENCODED_MESSAGE_SIZE + 5)
#define ENCODE_OFFSET COBS_ENCODE_SRC_OFFSET(MAX_DECODED_MESSAGE_SIZE)

class CobsMessage
{
public:
    virtual bool message_send_writer(const uint8_t *data, uint32_t length) = 0;
    virtual bool message_processor(const uint8_t *data, uint32_t length) = 0;
    virtual void debug_message_writer(const char *message) = 0;

    size_t debug_printf(const char *format, ...)
    {
        va_list argptr;
        va_start(argptr, format);
        int n = vsnprintf(debug_message, sizeof(debug_message), format, argptr);
        va_end(argptr);
        debug_message_writer(debug_message);
        return n;
    }

    bool send_message(uint8_t *data, uint32_t data_length)
    {
        //in-place encoding
        //data should be offset by received_message_buffer
        uint8_t *encoded_message_buffer = received_message_buffer;
        cobs_encode_result result = cobs_encode(encoded_message_buffer, MAX_ENCODED_MESSAGE_SIZE, data, data_length);
        if (result.status != COBS_ENCODE_OK)
        {
            debug_printf("message encode error: %d\n", result.status);
            return false;
        }
        // no need to memset zero.
        encoded_message_buffer[result.out_len] = 0;
        message_send_writer(encoded_message_buffer, (uint32_t)(result.out_len + 1));
        return true;
    }
    bool send_message(const char *string_message)
    {
        return send_message((uint8_t *)string_message, (uint32_t)(strlen(string_message) + 1));
    }
    bool decode_and_process_message(uint8_t *encoded_message, uint32_t encoded_message_length)
    {
        //in-place decoding
        uint8_t *decoded_message_buffer = received_message_buffer;
        cobs_decode_result result = cobs_decode(decoded_message_buffer, MAX_DECODED_MESSAGE_SIZE, encoded_message, encoded_message_length);
        if (result.status == COBS_DECODE_OK)
        {
            return message_processor(decoded_message_buffer, (uint32_t)(result.out_len));
        }
        else
        {
            debug_printf("message decode error: %d\n", result.status);
            return false;
        }
    }
    bool message_decoder_fill_data(uint8_t c)
    {
        if (c > 0)
        {
            if (n_received >= sizeof(received_message_buffer))
            {
                debug_printf("Error: receive buffer will overflow, abort.\n");
                n_received = 0;
                return false;
            }
            received_message_buffer[n_received] = c;
            n_received++;
            return true;
        }
        else
        {
            bool result = decode_and_process_message(received_message_buffer, n_received);
            n_received = 0;
            return result;
        }
    }

protected:
    uint8_t received_message_buffer[RECEIVE_BUFFER_SIZE];

private:
    uint32_t n_received = 0;
    char debug_message[250];
};

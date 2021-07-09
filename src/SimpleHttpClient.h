#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

//DigiCert Global Root CA
const char *rootCA =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDrzCCApegAwIBAgIQCDvgVpBCRrGhdWrJWZHHSjANBgkqhkiG9w0BAQUFADBh\n"
    "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
    "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBD\n"
    "QTAeFw0wNjExMTAwMDAwMDBaFw0zMTExMTAwMDAwMDBaMGExCzAJBgNVBAYTAlVT\n"
    "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
    "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IENBMIIBIjANBgkqhkiG\n"
    "9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4jvhEXLeqKTTo1eqUKKPC3eQyaKl7hLOllsB\n"
    "CSDMAZOnTjC3U/dDxGkAV53ijSLdhwZAAIEJzs4bg7/fzTtxRuLWZscFs3YnFo97\n"
    "nh6Vfe63SKMI2tavegw5BmV/Sl0fvBf4q77uKNd0f3p4mVmFaG5cIzJLv07A6Fpt\n"
    "43C/dxC//AH2hdmoRBBYMql1GNXRor5H4idq9Joz+EkIYIvUX7Q6hL+hqkpMfT7P\n"
    "T19sdl6gSzeRntwi5m3OFBqOasv+zbMUZBfHWymeMr/y7vrTC0LUq7dBMtoM1O/4\n"
    "gdW7jVg/tRvoSSiicNoxBN33shbyTApOB6jtSj1etX+jkMOvJwIDAQABo2MwYTAO\n"
    "BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUA95QNVbR\n"
    "TLtm8KPiGxvDl7I90VUwHwYDVR0jBBgwFoAUA95QNVbRTLtm8KPiGxvDl7I90VUw\n"
    "DQYJKoZIhvcNAQEFBQADggEBAMucN6pIExIK+t1EnE9SsPTfrgT1eXkIoyQY/Esr\n"
    "hMAtudXH/vTBH1jLuG2cenTnmCmrEbXjcKChzUyImZOMkXDiqw8cvpOp/2PV5Adg\n"
    "06O/nVsJ8dWO41P0jmP6P6fbtGbfYmbW0W5BjfIttep3Sp+dWOIrWcBAI+0tKIJF\n"
    "PnlUkiaY4IBIqDfv8NZ5YBberOgOzW6sRBc4L0na4UU+Krk2U886UAb3LujEV0ls\n"
    "YSEY1QSteDwsOoBrp+uvFRTp2InBuThs4pFsiv9kuXclVzDAGySj4dzp30d8tbQk\n"
    "CAUw7C29C79Fv1C5qfPrmAESrciIxpg0X40KPMbp1ZWVbd4=\n"
    "-----END CERTIFICATE-----\n";

struct HttpResult
{
    uint16_t code;
    uint16_t data_length;
};

class SimpleHttpClient
{
private:
    WiFiClientSecure *client = nullptr;

public:
    SimpleHttpClient()
    {
    }
    bool begin()
    {
        client = new WiFiClientSecure;
        if (client)
        {
            client->setCACert(rootCA);
            return true;
        }
        else
        {
            return false;
        }
    }
    void end()
    {
        delete client;
    }
    HttpResult http_get(const char *url, uint8_t *result, int result_length)
    {
        HTTPClient https;
        auto ret = https.begin(*client, url);
        if (ret)
        {
            uint16_t code = (uint16_t)(https.GET());
            if (code == HTTP_CODE_OK)
            {
                auto size = https.getSize();
                if (size == -1)
                {
                    //server must send Content-Length header. if not, abort.
                    return HttpResult{code, 0};
                }
                else if (size > (result_length - 1))
                {
                    //pay load size must small enough.
                    return HttpResult{code, 0};
                }
                else
                {
                    auto payload = https.getString();
                    payload.getBytes(result, result_length);
                    return HttpResult{code, (uint16_t)payload.length()};
                }
            }
            else
            {
                return HttpResult{code, 0};
            }
        }
        else
        {
            return HttpResult{0, 0};
        }
    }
    void self_test()
    {
        const char *url = "https://learndevops.cn/ip.php";
        char ip[512] = {0};
        auto result = http_get(url, (uint8_t *)ip, sizeof(ip));
        Serial.printf("result: code = %d, size = %d\n", result.code, result.data_length);
        if (result.code == HTTP_CODE_OK)
        {
            Serial.printf("ip: %s\n", ip);
        }
    }
};

extern SimpleHttpClient httpClient;

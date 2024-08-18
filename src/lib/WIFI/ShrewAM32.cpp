#include "ShrewAM32.h"
#include "common.h"

#if defined(BUILD_SHREW_AM32CONFIG)
#if defined(PLATFORM_ESP32)

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "driver/uart.h"

typedef struct
{
    int pin;
    uint8_t action;
    uint8_t* buffer1;
    uint32_t buffer1_len;
    uint32_t delay;
    uint8_t* buffer2;
    uint32_t buffer2_len;
}
am32_request_t;

enum
{
    AM32_ACTION_PIN_LIST, // reply with a list of pins
    AM32_ACTION_PIN_LOW,  // driven low
    AM32_ACTION_PIN_HIGH, // driven high
    AM32_ACTION_PIN_INIT, // pulled high and initialized as serial input
    AM32_ACTION_READ,
    AM32_ACTION_WRITE,
};

static int pin_num = -1;

void am32_setPinMode(int pin, bool isTx);
void am32_freeStruct(am32_request_t* x);
void am32_hexDecode(const char* str, uint8_t* outbuf, int len);
void WebUpdateSendContent(AsyncWebServerRequest *request);

void am32_handleIo(AsyncWebServerRequest *request)
{
    am32_request_t req_data = {0};
    req_data.pin = pin_num;

    int paramsNr = request->params();
    for (int i = 0; i < paramsNr; i++) {
        AsyncWebParameter* p = request->getParam(i);
        if (p->name().equalsIgnoreCase("pin")) {
            req_data.pin = (int)p->value().toInt();
            pin_num = req_data.pin;
        }
        if (p->name().equalsIgnoreCase("action")) {
            req_data.action = (uint8_t)p->value().toInt();
        }
        if (p->name().equalsIgnoreCase("delay")) {
            req_data.delay = (uint32_t)p->value().toInt();
        }
        if (p->name().equalsIgnoreCase("len1")) {
            uint32_t len = (uint32_t)p->value().toInt();
            if (len > 0) {
                req_data.buffer1 = (uint8_t*)malloc(len);
                if (req_data.buffer1) {
                    req_data.buffer1_len = len;
                }
            }
        }
        if (p->name().equalsIgnoreCase("len2")) {
            uint32_t len = (uint32_t)p->value().toInt();
            if (len > 0) {
                req_data.buffer2 = (uint8_t*)malloc(len);
                if (req_data.buffer2) {
                    req_data.buffer2_len = len;
                }
            }
        }
    }
    for (int i = 0; i < paramsNr; i++) {
        AsyncWebParameter* p = request->getParam(i);
        if (p->name().equalsIgnoreCase("data1")) {
            am32_hexDecode(p->value().c_str(), req_data.buffer1, req_data.buffer1_len);
        }
        if (p->name().equalsIgnoreCase("data2")) {
            am32_hexDecode(p->value().c_str(), req_data.buffer2, req_data.buffer2_len);
        }
    }

    switch (req_data.action)
    {
        case AM32_ACTION_PIN_LOW:
            pinMatrixOutDetach(req_data.pin, false, false);
            pinMatrixInDetach(req_data.pin, false, false);
            pinMode(req_data.pin, OUTPUT);
            digitalWrite(req_data.pin, LOW);
            break;
        case AM32_ACTION_PIN_HIGH:
            pinMatrixOutDetach(req_data.pin, false, false);
            pinMatrixInDetach(req_data.pin, true, false);
            pinMode(req_data.pin, OUTPUT);
            digitalWrite(req_data.pin, HIGH);
            break;
        case AM32_ACTION_PIN_INIT:
            Serial.end();
            Serial.begin(19200, SERIAL_8N1, req_data.pin, req_data.pin);
            pinMatrixOutDetach(req_data.pin, false, false);
            pinMatrixInDetach(req_data.pin, true, false);
            pinMode(req_data.pin, INPUT_PULLUP);
            am32_setPinMode(req_data.pin, false);
            break;
        case AM32_ACTION_PIN_LIST:
            {
                char pin_str[64];
                AsyncResponseStream *response = request->beginResponseStream("text/plain");

                if (GPIO_PIN_RCSIGNAL_TX >= 0)
                {
                    snprintf(pin_str, 62, "SERTX %d,", GPIO_PIN_RCSIGNAL_TX);
                    response->print(pin_str);
                }
                if (GPIO_PIN_RCSIGNAL_RX >= 0)
                {
                    snprintf(pin_str, 62, "SERRX %d,", GPIO_PIN_RCSIGNAL_RX);
                    response->print(pin_str);
                }

                for (int ch = 0 ; ch < GPIO_PIN_PWM_OUTPUTS_COUNT ; ++ch)
                {
                    int8_t pwm_pin = GPIO_PIN_PWM_OUTPUTS[ch];
                    snprintf(pin_str, 62, "PWM %d %d,", ch, pwm_pin);
                    response->print(pin_str);
                }
                request->send(response);
                am32_freeStruct(&req_data);
                return;
            }
        case AM32_ACTION_READ:
            {
                AsyncResponseStream *response = request->beginResponseStream("text/plain");
                bool has = false;
                char hex[3];
                while (Serial.available() > 0) {
                    sprintf(hex, "%02X", Serial.read());
                    response->write(hex, 2);
                    has = true;
                }
                if (has == false) {
                    sprintf(hex, "xx");
                    response->write(hex, 2);
                }
                request->send(response);
                am32_freeStruct(&req_data);
                return;
            }
        case AM32_ACTION_WRITE:
            am32_setPinMode(req_data.pin, true);
            for (int j = 0; j < req_data.buffer1_len; j++) {
                Serial.write((uint8_t)req_data.buffer1[j]);
            }
            uart_wait_tx_done(0, pdMS_TO_TICKS(100));
            if (req_data.delay > 0 && req_data.buffer2_len > 0) {
                delayMicroseconds(req_data.delay);
            }
            for (int j = 0; j < req_data.buffer2_len; j++) {
                Serial.write((uint8_t)req_data.buffer2[j]);
            }
            uart_wait_tx_done(0, pdMS_TO_TICKS(100));
            am32_setPinMode(req_data.pin, false);
            am32_freeStruct(&req_data);
            AsyncResponseStream *response = request->beginResponseStream("text/plain");
            response->write("ok", 2);
            request->send(response);
            return;
    }

    am32_freeStruct(&req_data);
}

void am32_setupServer(AsyncWebServer* srv)
{
    srv->on("/am32.html", WebUpdateSendContent);
    srv->on("/am32.js", WebUpdateSendContent);
    srv->on("/am32io", HTTP_POST, am32_handleIo);
}

void am32_setPinMode(int pin, bool isTx)
{
    if (isTx)
    {
        pinMatrixOutAttach(pin, U0TXD_OUT_IDX, false, false);
    }
    else
    {
        pinMatrixInAttach(pin, U0RXD_IN_IDX, false);
    }
}

void am32_freeStruct(am32_request_t* x) {
    if (x->buffer1_len > 0) {
        free(x->buffer1);
        x->buffer1_len = 0;
        x->buffer1 = NULL;
    }
    if (x->buffer2_len > 0) {
        free(x->buffer2);
        x->buffer2_len = 0;
        x->buffer2 = NULL;
    }
}

void am32_hexDecode(const char* str, uint8_t* outbuf, int len) {
    int slen = strlen(str);
    char tmp[3] = {0, 0, 0};
    for (int i = 0; i < len && i < slen / 2; i++)
    {
        int j = i * 2;
        tmp[0] = str[j];
        tmp[1] = str[j + 1];
        int k = strtol(tmp, NULL, 16);
        outbuf[i] = (uint8_t)k;
    }
}

/*
String am32_encodeHex(uint8_t* data, size_t length) {
    String result;
    result.reserve(length * 2);
    for (size_t i = 0; i < length; ++i) {
        char hex[3];
        sprintf(hex, "%02X", data[i]);
        result += hex;
    }
    return result;
}
*/

#else // PLATFORM_ESP32

#if defined(PLATFORM_ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include <ESPAsyncWebServer.h>

void am32_setupServer(AsyncWebServer* srv)
{
    // do nothing
}

#endif // PLATFORM_ESP32

#endif
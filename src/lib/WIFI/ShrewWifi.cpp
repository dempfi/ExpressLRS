#include "common.h"
#include "device.h"
#include "hwTimer.h"

#ifdef BUILD_SHREW_WIFI
#include "ShrewWifi.h"

#if defined(PLATFORM_ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#include <ESPAsyncWebServer.h>

#include <AsyncJson.h>
#include <ArduinoJson.h>
#if defined(PLATFORM_ESP8266)
#include <FS.h>
#else
#include <SPIFFS.h>
#endif

AsyncWebSocket ws("/shrew_ws");

static bool has_inited = false;
static uint32_t last_rx_time = 0;
static uint32_t last_ws_time = 0;

extern void WebUpdateSendContent(AsyncWebServerRequest *request);
extern void ICACHE_RAM_ATTR servoNewChannelsAvailable();
extern void ICACHE_RAM_ATTR crsfRCFrameAvailable();
extern bool connectionHasModelMatch;
#if defined(TARGET_TX)
extern uint32_t TLMpacketReported;
#endif

static void OnWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    uint32_t now = millis();
    if (type == WS_EVT_CONNECT) {
        //Serial.println("WebSocket client connected");
        if (has_inited == false) {
            #if defined(TARGET_TX)
            shrew_restartRadio();
            #endif
            has_inited = true;
        }
    } else if (type == WS_EVT_DISCONNECT) {
        //Serial.println("WebSocket client disconnected");
    } else if (type == WS_EVT_DATA) {
        bool gamepad_good;
        if ((gamepad_good = (memcmp(data, "CRSF", 4) == 0)) || memcmp(data, "crsf", 4) == 0) {
            uint16_t* channels16 = (uint16_t*)&(data[4]);
            int i, len2 = (len - 4)/2;
            if (gamepad_good && len2 == CRSF_NUM_CHANNELS)
            {
                last_rx_time = now;
                for (i = 0; i < len2 && i < CRSF_NUM_CHANNELS; i++) {
                    ChannelData[i] = channels16[i];
                }
                #if defined(TARGET_RX)
                connectionHasModelMatch = true;
                servoNewChannelsAvailable(); // servo PWM generator handles output frequency
                crsfRCFrameAvailable();      // serialIO loop handles output rate
                #elif defined(TARGET_TX)
                #endif
            }
            if ((now - last_ws_time) >= 100 && client->canSend()) {
                #if defined(TARGET_TX)
                if ((now - TLMpacketReported) <= 1000) {
                    // TODO: CRSF::LinkStatistics
                    client->text("OK");
                }
                else {
                    client->text("ok");
                }
                #else
                client->text("ok");
                #endif
                last_ws_time = now;
            }
        }
    }
}

static void OnCfgSave(AsyncWebServerRequest *request, JsonVariant &json)
{
    JsonObject jsonObj = json.as<JsonObject>();
    File file = SPIFFS.open("/shrewcfg.json", "w");
    if (!file) {
        //Serial.println("There was an error opening the file for writing");
        request->send(200, "application/json", "{\"result\":\"failed\"}");
        return;
    }
    if (serializeJson(jsonObj, file) == 0) {
        //Serial.println("Failed to write to file");
    }
    file.close();
    request->send(200, "application/json", "{\"result\":\"success\"}");
    request->client()->close();
}

static void OnCfgLoad(AsyncWebServerRequest *request)
{
    request->send(SPIFFS, "/shrewcfg.json", "application/json", true);
    request->client()->close();
}

void shrew_handleWebUpdate(uint32_t now)
{
    if (has_inited == false) {
        return;
    }
    if ((now - last_rx_time) >= 1000) {
        connectionHasModelMatch = false;
    }
}

void shrew_setupServer(AsyncWebServer* srv)
{
    srv->on("/shrew.html", WebUpdateSendContent);
    srv->on("/shrew.js", WebUpdateSendContent);
    srv->on("/joy.js", WebUpdateSendContent);
    srv->on("/shrewcfgload", OnCfgLoad);
    srv->addHandler(new AsyncCallbackJsonWebHandler("/shrewcfgsave", OnCfgSave));
    ws.onEvent(OnWsEvent);
    srv->addHandler(&ws);
}

uint32_t shrew_getLastDataTime()
{
    return last_rx_time * 1000;
}

extern void ResetPower();

void shrew_restartRadio()
{
    Radio.Begin();
    ResetPower();
    hwTimer::resume();
    connectionState = noCrossfire;
}

#else // BUILD_SHREW_WIFI

#endif

extern device_t ServoOut_device;

static bool servos_initialized = false;
void shrew_markServosInitialized(bool x) {
    servos_initialized = x;
}

bool shrew_isActive() {
    #ifdef BUILD_SHREW_WIFI
    if (has_inited)
    {
        if (servos_initialized == false) {
            ServoOut_device.start();
        }
        return true;
    }
    #endif
    return false;
}
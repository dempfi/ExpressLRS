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

static void OnWsEvent(AsyncWebSocket* server, AsyncWebSocketClient* client, AwsEventType type, void* arg, uint8_t* data, size_t len)
{
    uint32_t now = millis();
    if (type == WS_EVT_CONNECT) {
        //Serial.println("WebSocket client connected");
        if (has_inited == false) {
            // TODO: do stuff here
            has_inited = true;
        }
    } else if (type == WS_EVT_DISCONNECT) {
        //Serial.println("WebSocket client disconnected");
    } else if (type == WS_EVT_DATA) {
        last_rx_time = now;
        uint16_t* channels16 = (uint16_t*)data;
        int i, len2 = len/2;
        for (i = 0; i < len2; i++) {
            ChannelData[i] = channels16[i];
        }
        #if defined(TARGET_RX)
        connectionHasModelMatch = true;
        servoNewChannelsAvailable(); // servo PWM generator handles output frequency
        crsfRCFrameAvailable();      // serialIO loop handles output rate
        #elif defined(TARGET_TX)
        #endif
        if ((now - last_ws_time) >= 100) {
            client->text("ok");
            last_ws_time = now;
        }
    }
}

static void OnCfgSave(AsyncWebServerRequest *request, JsonVariant &json)
{
    JsonObject jsonObj = json.as<JsonObject>();
    File file = SPIFFS.open("/shrewcfg.json", "w");
    if (!file) {
        //Serial.println("There was an error opening the file for writing");
        return;
    }
    if (serializeJson(jsonObj, file) == 0) {
        //Serial.println("Failed to write to file");
    }
    file.close();
    request->send(200, "application/json", "{\"result\":\"success\"}");
}

static void OnCfgLoad(AsyncWebServerRequest *request)
{
    request->send(SPIFFS, "/shrewcfg.json", "application/json", true);
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
    srv->on("/shrew_cfgload", OnCfgLoad);
    srv->addHandler(new AsyncCallbackJsonWebHandler("/shrew_cfgsave", OnCfgSave));
    ws.onEvent(OnWsEvent);
    srv->addHandler(&ws);
}

uint32_t shrew_getLastDataTime()
{
    return last_rx_time * 1000;
}
#else // BUILD_SHREW_WIFI

#endif
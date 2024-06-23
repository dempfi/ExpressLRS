#pragma once

#ifdef BUILD_SHREW_WIFI

static bool shrew_wifi_started = false;
extern void SetRFLinkRate(uint8_t);

void shrew_startWifi()
{
  uint32_t now = millis();
  #ifdef TARGET_TX
  SetRFLinkRate(config.GetRate());
  hwTimer::resume();
  #endif
  webserverPreventAutoStart = true;
  WiFi.persistent(false);
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  strcpy(station_ssid, firmwareOptions.home_wifi_ssid);
  strcpy(station_password, firmwareOptions.home_wifi_password);
  if (station_ssid[0] == 0) {
    changeTime = now;
    changeMode = WIFI_AP;
  }
  else {
    changeTime = now;
    changeMode = WIFI_STA;
  }
  laststatus = WL_DISCONNECTED;
  wifiStarted = true;
  connectionState = disconnected;
  shrew_wifi_started = true;
}

bool shrew_hasWifiStarted()
{
  return shrew_wifi_started;
}

#endif

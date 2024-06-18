#pragma once

#include "common.h"
#include <stdint.h>

#include <ESPAsyncWebServer.h>

void shrew_setupServer(AsyncWebServer* srv);
void shrew_handleWebUpdate(uint32_t now);
uint32_t shrew_getLastDataTime();
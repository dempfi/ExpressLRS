#pragma once
#include "common.h"
#include "devServoOutput.h"

#define HBRIDGE_PIN_A1 10
#define HBRIDGE_PIN_A2 9
#define HBRIDGE_PIN_B1 17
#define HBRIDGE_PIN_B2 16
#define HBRIDGE_PWM_FREQ 24000

void hbridge_init(void);
void hbridge_failsafe(void);
void hbridge_update(unsigned long now);

void shrew_markServosInitialized(bool);
bool shrew_isActive(void);

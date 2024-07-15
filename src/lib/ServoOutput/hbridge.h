#pragma once
#include "common.h"
#include "devServoOutput.h"

#define HBRIDGE_PIN_A1 10
#define HBRIDGE_PIN_A2 9
#define HBRIDGE_PIN_B1 19
#define HBRIDGE_PIN_B2 22
#define HBRIDGE_PIN_NSLEEP 3 // use with Shrew development board
//#define HBRIDGE_PIN_NSLEEP 4 // use with Shrew Mega Revision 0
#define HBRIDGE_PWM_FREQ 24000U

void hbridge_init(void);
void hbridge_failsafe(void);
void hbridge_update(unsigned long now);

void shrew_markServosInitialized(bool);
bool shrew_isActive(void);

#if (defined(GPIO_PIN_PWM_OUTPUTS) && defined(PLATFORM_ESP32) && defined(BUILD_SHREW_HBRIDGE))

#include "hbridge.h"
#include "devServoOutput.h"
#include "PWM.h"
#include "CRSF.h"
#include "logging.h"

// these are defined from the shrew.ini file
//#define HBRIDGE_DRV8244
//#define HBRIDGE_DRV8231

#if defined(HBRIDGE_DRV8244)
#define HBRIDGE_SLEEP_VAL    1000
#elif defined(HBRIDGE_DRV8231)
#define HBRIDGE_SLEEP_VAL    0
#else
#error
#endif
#define HBRIDGE_FULLON_VAL    (1000 - HBRIDGE_SLEEP_VAL)

enum {
    HBRIDGE_IDX_A1 = 0,
    HBRIDGE_IDX_A2 = 1,
    HBRIDGE_IDX_B1 = 2,
    HBRIDGE_IDX_B2 = 3,
};

static pwm_channel_t hbridge_channels[4];
static bool has_init = false;
static unsigned long move_time = 0;

void hbridge_init(void)
{
    if (has_init) {
        return;
    }
    pinMode(HBRIDGE_PIN_A1, OUTPUT);
    digitalWrite(HBRIDGE_PIN_A1, LOW);
    hbridge_channels[HBRIDGE_IDX_A1] = PWM.allocate(HBRIDGE_PIN_A1, HBRIDGE_PWM_FREQ);
    pinMode(HBRIDGE_PIN_A2, OUTPUT);
    digitalWrite(HBRIDGE_PIN_A2, LOW);
    hbridge_channels[HBRIDGE_IDX_A2] = PWM.allocate(HBRIDGE_PIN_A2, HBRIDGE_PWM_FREQ);
    pinMode(HBRIDGE_PIN_B1, OUTPUT);
    digitalWrite(HBRIDGE_PIN_B1, LOW);
    hbridge_channels[HBRIDGE_IDX_B1] = PWM.allocate(HBRIDGE_PIN_B1, HBRIDGE_PWM_FREQ);
    pinMode(HBRIDGE_PIN_B2, OUTPUT);
    digitalWrite(HBRIDGE_PIN_B2, LOW);
    hbridge_channels[HBRIDGE_IDX_B2] = PWM.allocate(HBRIDGE_PIN_B2, HBRIDGE_PWM_FREQ);
    has_init = true;
    #if defined(HBRIDGE_DRV8244) && defined(HBRIDGE_PIN_NSLEEP)
    pinMode(HBRIDGE_PIN_NSLEEP, OUTPUT);
    digitalWrite(HBRIDGE_PIN_NSLEEP, LOW);
    delayMicroseconds(5);
    digitalWrite(HBRIDGE_PIN_NSLEEP, HIGH);
    #endif
    hbridge_failsafe();
}

void hbridge_failsafe(void)
{
    if (has_init == false) {
        return;
    }
    DBGLN("hbridge_failsafe");
    PWM.setDuty(hbridge_channels[HBRIDGE_IDX_A1], HBRIDGE_SLEEP_VAL);
    PWM.setDuty(hbridge_channels[HBRIDGE_IDX_A2], HBRIDGE_SLEEP_VAL);
    PWM.setDuty(hbridge_channels[HBRIDGE_IDX_B1], HBRIDGE_SLEEP_VAL);
    PWM.setDuty(hbridge_channels[HBRIDGE_IDX_B2], HBRIDGE_SLEEP_VAL);
    move_time = 0;
}

void hbridge_setDuty(pwm_channel_t ch, signed int data)
{
    PWM.setDuty(ch, data < 0 ? 0 : (data > 1000 ? 1000 : data));
}

void hbridge_update(unsigned long now)
{
    unsigned ch1 = ChannelData[0];
    unsigned ch2 = ChannelData[1];

    // note: setDuty expects duty 0-1000, internally it uses mcpwm_set_duty which accepts float 0-100, there's a divide by 10.0f internally
    // note: both pins H is means driver is in standby/hi-z

    bool stdby = ((now - move_time) >= 5000);

    if (ch1 > CRSF_CHANNEL_VALUE_MID) {
        move_time = now;
        hbridge_setDuty(hbridge_channels[HBRIDGE_IDX_A2], fmap(ch1, CRSF_CHANNEL_VALUE_MID, CRSF_CHANNEL_VALUE_MAX, HBRIDGE_FULLON_VAL, HBRIDGE_SLEEP_VAL));
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_A1], HBRIDGE_FULLON_VAL);
    }
    else if (ch1 < CRSF_CHANNEL_VALUE_MID) {
        move_time = now;
        hbridge_setDuty(hbridge_channels[HBRIDGE_IDX_A1], fmap(ch1, CRSF_CHANNEL_VALUE_MIN, CRSF_CHANNEL_VALUE_MID, HBRIDGE_SLEEP_VAL, HBRIDGE_FULLON_VAL));
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_A2], HBRIDGE_FULLON_VAL);
    }
    else if (stdby) {
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_A1], HBRIDGE_SLEEP_VAL);
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_A2], HBRIDGE_SLEEP_VAL);
    }
    else {
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_A1], HBRIDGE_FULLON_VAL);
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_A2], HBRIDGE_FULLON_VAL);
    }

    if (ch2 > CRSF_CHANNEL_VALUE_MID) {
        move_time = now;
        hbridge_setDuty(hbridge_channels[HBRIDGE_IDX_B2], fmap(ch2, CRSF_CHANNEL_VALUE_MID, CRSF_CHANNEL_VALUE_MAX, HBRIDGE_FULLON_VAL, HBRIDGE_SLEEP_VAL));
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_B1], HBRIDGE_FULLON_VAL);
    }
    else if (ch2 < CRSF_CHANNEL_VALUE_MID) {
        move_time = now;
        hbridge_setDuty(hbridge_channels[HBRIDGE_IDX_B1], fmap(ch2, CRSF_CHANNEL_VALUE_MIN, CRSF_CHANNEL_VALUE_MID, HBRIDGE_SLEEP_VAL, HBRIDGE_FULLON_VAL));
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_B2], HBRIDGE_FULLON_VAL);
    }
    else if (stdby) {
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_B1], HBRIDGE_SLEEP_VAL);
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_B2], HBRIDGE_SLEEP_VAL);
    }
    else {
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_B1], HBRIDGE_FULLON_VAL);
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_B2], HBRIDGE_FULLON_VAL);
    }
}

#endif

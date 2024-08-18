#if (defined(GPIO_PIN_PWM_OUTPUTS) && defined(PLATFORM_ESP32))

#include "hbridge.h"
#include "devServoOutput.h"
#include "PWM.h"
#include "CRSF.h"
#include "logging.h"

enum {
    HBRIDGE_IDX_A1 = 0,
    HBRIDGE_IDX_A2 = 1,
    HBRIDGE_IDX_B1 = 2,
    HBRIDGE_IDX_B2 = 3,
};

static pwm_channel_t hbridge_channels[4];
static bool has_init = false;
static unsigned long move_time = 0;

static uint8_t hbridge_pin_a1;
static uint8_t hbridge_pin_a2;
static uint8_t hbridge_pin_b1;
static uint8_t hbridge_pin_b2;
static uint32_t hbridge_sleep_val;
static uint32_t hbridge_full_val;

void hbridge_init(void)
{
    if (has_init) {
        return;
    }
    if (firmwareOptions.shrew <= 0) {
        DBGLN("hbridge not shrew");
        return;
    }

    if (firmwareOptions.shrew == 2 || firmwareOptions.shrew == 4) { // mega/pro
        hbridge_sleep_val = 1000;
    }
    else { // mini/lite
        hbridge_sleep_val = 0;
    }

    hbridge_full_val = 1000 - hbridge_sleep_val;

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    if (chip_info.revision == 3) { // PICO-V3
        hbridge_pin_a1 = 16;
        hbridge_pin_a2 = 17;
        hbridge_pin_b1 = 13;
        hbridge_pin_b2 = 27;
    }
    else { // PICO-D4
        hbridge_pin_a1 = 9;
        hbridge_pin_a2 = 10;
        if (firmwareOptions.shrew <= 2) {
            hbridge_pin_b1 = 13;
            hbridge_pin_b2 = 27;
        }
        else {
            hbridge_pin_b1 = 19;
            hbridge_pin_b2 = 22;
        }
    }

    pinMode(hbridge_pin_a1, OUTPUT);
    digitalWrite(hbridge_pin_a1, LOW);
    hbridge_channels[HBRIDGE_IDX_A1] = PWM.allocate(hbridge_pin_a1, HBRIDGE_PWM_FREQ);
    pinMode(hbridge_pin_a2, OUTPUT);
    digitalWrite(hbridge_pin_a2, LOW);
    hbridge_channels[HBRIDGE_IDX_A2] = PWM.allocate(hbridge_pin_a2, HBRIDGE_PWM_FREQ);
    pinMode(hbridge_pin_b1, OUTPUT);
    digitalWrite(hbridge_pin_b1, LOW);
    hbridge_channels[HBRIDGE_IDX_B1] = PWM.allocate(hbridge_pin_b1, HBRIDGE_PWM_FREQ);
    pinMode(hbridge_pin_b2, OUTPUT);
    digitalWrite(hbridge_pin_b2, LOW);
    hbridge_channels[HBRIDGE_IDX_B2] = PWM.allocate(hbridge_pin_b2, HBRIDGE_PWM_FREQ);
    has_init = true;
    hbridge_failsafe();
    DBGLN("hbridge finished init");
}

void hbridge_failsafe(void)
{
    if (has_init == false) {
        return;
    }
    DBGLN("hbridge_failsafe");
    PWM.setDuty(hbridge_channels[HBRIDGE_IDX_A1], hbridge_sleep_val);
    PWM.setDuty(hbridge_channels[HBRIDGE_IDX_A2], hbridge_sleep_val);
    PWM.setDuty(hbridge_channels[HBRIDGE_IDX_B1], hbridge_sleep_val);
    PWM.setDuty(hbridge_channels[HBRIDGE_IDX_B2], hbridge_sleep_val);
    move_time = 0;
}

void hbridge_setDuty(pwm_channel_t ch, signed int data)
{
    PWM.setDuty(ch, data < 0 ? 0 : (data > 1000 ? 1000 : data));
}

void hbridge_update(unsigned long now)
{
    if (has_init == false) {
        return;
    }

    unsigned ch1 = ChannelData[0];
    unsigned ch2 = ChannelData[1];

    // note: setDuty expects duty 0-1000, internally it uses mcpwm_set_duty which accepts float 0-100, there's a divide by 10.0f internally
    // note: both pins H is means driver is in standby/hi-z

    bool stdby = ((now - move_time) >= 5000);

    if (ch1 > CRSF_CHANNEL_VALUE_MID) {
        move_time = now;
        hbridge_setDuty(hbridge_channels[HBRIDGE_IDX_A2], fmap(ch1, CRSF_CHANNEL_VALUE_MID, CRSF_CHANNEL_VALUE_MAX, hbridge_full_val, hbridge_sleep_val));
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_A1], hbridge_full_val);
    }
    else if (ch1 < CRSF_CHANNEL_VALUE_MID) {
        move_time = now;
        hbridge_setDuty(hbridge_channels[HBRIDGE_IDX_A1], fmap(ch1, CRSF_CHANNEL_VALUE_MIN, CRSF_CHANNEL_VALUE_MID, hbridge_sleep_val, hbridge_full_val));
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_A2], hbridge_full_val);
    }
    else if (stdby) {
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_A1], hbridge_sleep_val);
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_A2], hbridge_sleep_val);
    }
    else {
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_A1], hbridge_full_val);
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_A2], hbridge_full_val);
    }

    if (ch2 > CRSF_CHANNEL_VALUE_MID) {
        move_time = now;
        hbridge_setDuty(hbridge_channels[HBRIDGE_IDX_B2], fmap(ch2, CRSF_CHANNEL_VALUE_MID, CRSF_CHANNEL_VALUE_MAX, hbridge_full_val, hbridge_sleep_val));
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_B1], hbridge_full_val);
    }
    else if (ch2 < CRSF_CHANNEL_VALUE_MID) {
        move_time = now;
        hbridge_setDuty(hbridge_channels[HBRIDGE_IDX_B1], fmap(ch2, CRSF_CHANNEL_VALUE_MIN, CRSF_CHANNEL_VALUE_MID, hbridge_sleep_val, hbridge_full_val));
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_B2], hbridge_full_val);
    }
    else if (stdby) {
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_B1], hbridge_sleep_val);
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_B2], hbridge_sleep_val);
    }
    else {
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_B1], hbridge_full_val);
        PWM.setDuty(hbridge_channels[HBRIDGE_IDX_B2], hbridge_full_val);
    }
}

#endif

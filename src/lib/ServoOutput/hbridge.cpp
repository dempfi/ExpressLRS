#if defined(GPIO_PIN_PWM_OUTPUTS)

#include "hbridge.h"
#include "devServoOutput.h"
#include "PWM.h"
#include "CRSF.h"

static pwm_channel_t hbridge_channels[4];
static bool has_init = false;
static unsigned long move_time = 0;

void hbridge_init(void)
{
    if (has_init) {
        return;
    }
    hbridge_channels[0] = PWM.allocate(HBRIDGE_PIN_A1, HBRIDGE_PWM_FREQ);
    hbridge_channels[1] = PWM.allocate(HBRIDGE_PIN_A2, HBRIDGE_PWM_FREQ);
    hbridge_channels[2] = PWM.allocate(HBRIDGE_PIN_B1, HBRIDGE_PWM_FREQ);
    hbridge_channels[3] = PWM.allocate(HBRIDGE_PIN_B2, HBRIDGE_PWM_FREQ);
    hbridge_failsafe();
    has_init = true;
}

void hbridge_failsafe(void)
{
    if (has_init == false) {
        return;
    }
    PWM.setDuty(HBRIDGE_PIN_A1, 1000);
    PWM.setDuty(HBRIDGE_PIN_A2, 1000);
    PWM.setDuty(HBRIDGE_PIN_B1, 1000);
    PWM.setDuty(HBRIDGE_PIN_B2, 1000);
    move_time = 0;
}

void hbridge_update(unsigned long now)
{
    unsigned ch1 = ChannelData[0];
    unsigned ch2 = ChannelData[1];

    // note: setDuty expects duty 0-1000, internally it uses mcpwm_set_duty which accepts float 0-100, there's a divide by 10.0f internally
    // note: both pins H is means driver is in standby/hi-z

    bool stdby = ((now - move_time) >= 5000);

    if (ch1 > 1500) {
        move_time = now;
        PWM.setDuty(HBRIDGE_PIN_A1, 0);
        PWM.setDuty(HBRIDGE_PIN_A2, fmap(ch1, CRSF_CHANNEL_VALUE_MIN, CRSF_CHANNEL_VALUE_MAX, 0, 1000));
    }
    else if (ch1 < 1500) {
        move_time = now;
        PWM.setDuty(HBRIDGE_PIN_A1, 1000);
        PWM.setDuty(HBRIDGE_PIN_A2, fmap(ch1, CRSF_CHANNEL_VALUE_MIN, CRSF_CHANNEL_VALUE_MAX, 1000, 0));
    }
    else if (stdby) {
        PWM.setDuty(HBRIDGE_PIN_A1, 1000);
        PWM.setDuty(HBRIDGE_PIN_A2, 1000);
    }
    else {
        PWM.setDuty(HBRIDGE_PIN_A1, 0);
        PWM.setDuty(HBRIDGE_PIN_A2, 0);
    }

    if (ch2 > 1500) {
        move_time = now;
        PWM.setDuty(HBRIDGE_PIN_B1, 0);
        PWM.setDuty(HBRIDGE_PIN_B2, fmap(ch2, CRSF_CHANNEL_VALUE_MIN, CRSF_CHANNEL_VALUE_MAX, 0, 1000));
    }
    else if (ch2 < 1500) {
        move_time = now;
        PWM.setDuty(HBRIDGE_PIN_B1, 1000);
        PWM.setDuty(HBRIDGE_PIN_B2, fmap(ch2, CRSF_CHANNEL_VALUE_MIN, CRSF_CHANNEL_VALUE_MAX, 1000, 0));
    }
    else if (stdby) {
        PWM.setDuty(HBRIDGE_PIN_B1, 1000);
        PWM.setDuty(HBRIDGE_PIN_B2, 1000);
    }
    else {
        PWM.setDuty(HBRIDGE_PIN_B1, 0);
        PWM.setDuty(HBRIDGE_PIN_B2, 0);
    }
}

#endif

#include "ShrewCfg.h"

#if defined(TARGET_RX)

void shrew_appendDefaults(RxConfig* cfg, rx_config_t* rxcfg)
{
    #if defined(GPIO_PIN_PWM_OUTPUTS)
    for (unsigned int ch = 0; ch < PWM_MAX_CHANNELS - 2; ch++)
    {
        rx_config_pwm_t *pwm = &(rxcfg->pwmChannels)[ch];
        pwm->val.inputChannel += 2;
        pwm->val.failsafe = 0;
    }
    #endif
}

#endif

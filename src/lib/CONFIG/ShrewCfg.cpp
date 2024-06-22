#include "ShrewCfg.h"

#if defined(TARGET_RX)

void shrew_appendDefaults(RxConfig* cfg, rx_config_t* rxcfg)
{
    #if defined(GPIO_PIN_PWM_OUTPUTS)
    for (unsigned int ch = 0; ch < PWM_MAX_CHANNELS - 2; ch++)
    {
        rx_config_pwm_t *pwm = &(rxcfg->pwmChannels)[ch];
        #if defined(BUILD_SHREW_HBRIDGE)
        pwm->val.inputChannel += 2;
        #endif
        pwm->val.failsafeMode = PWMFAILSAFE_NO_PULSES;
    }
    #endif
}

#endif

#if defined(TARGET_TX)

void shrew_appendDefaults(TxConfig* cfg, tx_config_t* txcfg)
{
}

#endif

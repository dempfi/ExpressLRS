#include "ShrewCfg.h"
#if defined(PLATFORM_ESP8266)
#include <FS.h>
#else
#include <SPIFFS.h>
#endif

#if defined(TARGET_RX)

void shrew_appendDefaults(RxConfig* cfg, rx_config_t* rxcfg)
{
    #if defined(GPIO_PIN_PWM_OUTPUTS)
    for (unsigned int ch = 0; ch < PWM_MAX_CHANNELS - 2; ch++)
    {
        rx_config_pwm_t *pwm = &(rxcfg->pwmChannels)[ch];
        if (firmwareOptions.shrew != 0) { // if shrew is a brushed ESC, then the first two channels are already used for driving
            pwm->val.inputChannel += 2;
        }
        pwm->val.failsafeMode = PWMFAILSAFE_NO_PULSES;
    }
    #endif

    rxcfg->locked_datarate = firmwareOptions.locked_datarate;
    if (firmwareOptions.permanent_binding) {
        rxcfg->bindStorage = BINDSTORAGE_PERMANENT;
    }
}

void shrew_cfgReset()
{
    // SPIFFS will have already been begun
    SPIFFS.remove("/options.json");
    SPIFFS.remove("/hardware.json");
}

#endif

#if defined(TARGET_TX)

void shrew_appendDefaults(TxConfig* cfg, tx_config_t* txcfg)
{
}

#endif

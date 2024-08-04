#pragma once

#include "config.h"
#include "options.h"

#if defined(TARGET_RX)
void shrew_appendDefaults(RxConfig* cfg, rx_config_t* rxcfg);
void shrew_cfgReset();
#endif
#if defined(TARGET_TX)
void shrew_appendDefaults(TxConfig* cfg, tx_config_t* txcfg);
#endif

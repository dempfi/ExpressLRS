#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include "common.h"
#include "options.h"

#ifdef BUILD_SHREW_ADCLUT

#define vbat_float_t double
#define vbat_float_round(x) lround(x)

typedef struct {
    uint16_t x;
    uint16_t y;
} tbl_entry_t;

static const tbl_entry_t lut[] = {
    #ifdef SHREW_ADC_USE_ZENER_3V6
    {614, 5067},
    {725, 6052},
    {841, 7064},
    {960, 8076},
    {1075, 9070},
    {1190, 10098},
    {1414, 12107},
    {1703, 15147},
    {1856, 17178},
    {2028, 20176},
    {2156, 23228},
    {2219, 25250},
    {2302, 28266},
    {2349, 30300},
    #else // no zener diode
    {589, 4974},
    {710, 6041},
    {827, 7060},
    {942, 8070},
    {1063, 9088},
    {1175, 10071},
    {1762, 15178},
    {2336, 20244},
    #endif
};

uint16_t shrew_last_vbat = 0;

static vbat_float_t shrewvbat_interpolate(const tbl_entry_t* table, int size, uint16_t x) {
    int i = 0;
    for (i = 0; i < size - 1; i++) {
        if (table[i].x <= x && table[i + 1].x >= x) {
            vbat_float_t slope = ((vbat_float_t)(table[i + 1].y - table[i].y)) / ((vbat_float_t)(table[i + 1].x - table[i].x));
            vbat_float_t y = table[i].y + slope * ((vbat_float_t)(x - table[i].x));
            return y;
        }
    }
    // If x is out of range in the table, perform extrapolation.
    if (x < table[0].x) {
        // Extrapolate using the first two entries
        vbat_float_t slope = ((vbat_float_t)(table[1].y - table[0].y)) / ((vbat_float_t)(table[1].x - table[0].x));
        vbat_float_t y = table[0].y + slope * ((vbat_float_t)(x - table[0].x));
        return y;
    } else {
        // Extrapolate using the last two entries
        vbat_float_t slope = ((vbat_float_t)(table[size - 1].y - table[size - 2].y)) / ((vbat_float_t)(table[size - 1].x - table[size - 2].x));
        vbat_float_t y = table[size - 2].y + slope * ((vbat_float_t)(x - table[size - 2].x));
        return y;
    }
}

int32_t shrewvbat_get(uint32_t x) {
    int lut_size = sizeof(lut) / sizeof(tbl_entry_t);
    vbat_float_t y = shrewvbat_interpolate(lut, lut_size, x);
    shrew_last_vbat = (uint32_t)vbat_float_round(y / 100.0);
    return shrew_last_vbat;
}

bool shrewvbat_canWifi() {
    if (shrew_last_vbat < 58) {
        return true;
    }
    if (connectionState == wifiUpdate) {
        return true;
    }
    #ifdef BUILD_SHREW_HBRIDGE
    if (firmwareOptions.shrew == 5 || firmwareOptions.shrew == 3) {
        //return false;
    }
    #endif
    return true;
}

#else
bool shrewvbat_canWifi() {
    return true;
}
#endif

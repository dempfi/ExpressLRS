#include <stdint.h>
#include <stdlib.h>

#ifdef BUILD_SHREW_ADCLUT

#define vbat_float_t double
#define vbat_float_round(x) lround(x)

typedef struct {
    vbat_float_t x;
    vbat_float_t y;
} tbl_entry_t;

static const tbl_entry_t lut[] = {
    {600, 5000},
    {710, 6000},
    {825, 7000},
    {942, 8000},
    {1060, 9000},
    {1175, 10000},
    {1404, 12000},
    {1686, 15000},
    {1836, 17000},
};

static vbat_float_t shrewvbat_interpolate(tbl_entry_t* table, int size, vbat_float_t x) {
    int i = 0;
    for (i = 0; i < size - 1; i++) {
        if (table[i].x <= x && table[i + 1].x >= x) {
            vbat_float_t slope = (table[i + 1].y - table[i].y) / (table[i + 1].x - table[i].x);
            vbat_float_t y = table[i].y + slope * (x - table[i].x);
            return y;
        }
    }
    // If x is out of range in the table, perform extrapolation.
    if (x < table[0].x) {
        // Extrapolate using the first two entries
        vbat_float_t slope = (table[1].y - table[0].y) / (table[1].x - table[0].x);
        vbat_float_t y = table[0].y + slope * (x - table[0].x);
        return y;
    } else {
        // Extrapolate using the last two entries
        vbat_float_t slope = (table[size - 1].y - table[size - 2].y) / (table[size - 1].x - table[size - 2].x);
        vbat_float_t y = table[size - 2].y + slope * (x - table[size - 2].x);
        return y;
    }
}

uint32_t shrewvbat_get(uint32_t x) {
    int lut_size = sizeof(table) / sizeof(Entry);
    vbat_float_t y = shrewvbat_interpolate(lut, lut_size, x);
    return (uint32_t)vbat_float_round(y);
}

#endif

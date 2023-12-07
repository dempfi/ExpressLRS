#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

enum
{
    AM32RET_SUCCESS,
    AM32RET_ERR_NOTREADY_NOPIN,
    AM32RET_ERR_TIMEOUT,
    AM32RET_ERR_BADCRC,
    AM32RET_ERR_BADACK,
    AM32RET_ERR_INVALID_CMD,
    AM32RET_ERR_INVALID_ARG,
};

int am32_handleRequest(char cmd, int ch, uint32_t addr, uint8_t* data, int* datacnt);

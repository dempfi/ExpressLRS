#include "am32.h"
#include "SoftwareSerial.h"
#include "devServoOutput.h"

static SoftwareSerial* ser = NULL;
static int prev_ch = 0;

static int am32_receive(uint8_t* buffer, int limit);
static int am32_verify(uint8_t* buffer, int cnt);
static void am32_appendCrc(uint8_t* buffer, int cnt);
static uint16_t am32_crc(uint8_t* buffer, int cnt);

static int am32_cmdAddress(uint16_t addr);
static int am32_cmdBufferLen(uint8_t datacnt);
static int am32_cmdPayload(uint8_t* data, uint8_t datacnt);
static int am32_cmdFlash(void);
static int am32_cmdRead(uint8_t* data, uint8_t datacnt);

enum {
    AM32REPLY_ACK = 0x30,
};

int am32_handleRequest(char cmd, int ch, uint32_t addr, uint8_t* data, int* datacnt)
{
    if (cmd == 's' || ch != prev_ch) {
        // start, simply initialize the pin
        if (ser != NULL) {
            ser->end();
            delete ser;
            ser = NULL;
        }
        int pin = servoChannelToPin(ch);
        ser = new SoftwareSerial(pin, pin, false);
        ser->begin(19200);
        ser->enableRxGPIOPullUp(true);
        ser->enableRx(true);
        prev_ch = ch;
        return AM32RET_SUCCESS;
    }

    if (ser == NULL) {
        return AM32RET_ERR_NOTREADY_NOPIN;
    }

    if (data == NULL || datacnt == NULL) {
        return AM32RET_ERR_INVALID_CMD;
    }

    while (ser->available()) {
        ser->read();
    }

    int _datacnt = *datacnt;

    if (cmd == 'q') {
        // query ESC
        static const uint8_t pkt[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0D, 0x42, 0x4C, 0x48, 0x65, 0x6C, 0x69, 0xF4, 0x7D };
        static const int pktlen = 21;
        ser->write((const char*)pkt, (size_t)pktlen);
        int rlen = am32_receive(data, 32);
        if (rlen > 0) {
            *datacnt = rlen;
            return AM32RET_SUCCESS;
        }
        else {
            return AM32RET_ERR_TIMEOUT;
        }
    }
    else if (cmd == 'w') {
        // write EEPROM
        int ret;
        if ((ret = am32_cmdAddress(addr)) != AM32RET_SUCCESS) {
            return ret;
        }
        if ((ret = am32_cmdBufferLen(_datacnt)) != AM32RET_SUCCESS) {
            return ret;
        }
        if ((ret = am32_cmdPayload(data, _datacnt)) != AM32RET_SUCCESS) {
            return ret;
        }
        if ((ret = am32_cmdFlash()) != AM32RET_SUCCESS) {
            return ret;
        }
        return AM32RET_SUCCESS;
    }
    else if (cmd == 'r') {
        // read EEPROM
        int ret;
        if ((ret = am32_cmdAddress(addr)) != AM32RET_SUCCESS) {
            return ret;
        }
        if ((ret = am32_cmdRead(data, _datacnt)) != AM32RET_SUCCESS) {
            return ret;
        }
        return AM32RET_SUCCESS;
    }
    return AM32RET_ERR_INVALID_CMD;
}

static int am32_receive(uint8_t* buffer, int limit)
{
    ser->enableRx(true);
    uint32_t now = millis();
    uint32_t start_time = now;
    int i = 0;
    while (((now = millis()) - start_time) < 200)
    {
        if (ser->available() > 0)
        {
            buffer[i] = ser->read();
            i++;
            break;
        }
        yield();
    }
    if (i == 0) {
        return -1;
    }
    start_time = millis();
    while (((now = millis()) - start_time) < 50 && i < limit)
    {
        while (ser->available() > 0 && i < limit)
        {
            start_time = millis();
            buffer[i] = ser->read();
            i++;
            yield();
        }
        yield();
    }
    return i;
}

static int am32_verify(uint8_t* buffer, int cnt)
{
    uint16_t crc = am32_crc(buffer, cnt - 2);
    uint8_t* pcrc = (uint8_t*)&crc;
    if (pcrc[0] == buffer[cnt - 3] && pcrc[1] == buffer[cnt - 2]) {
        if (buffer[cnt - 2] == AM32REPLY_ACK) {
            return AM32RET_SUCCESS;
        }
        else {
            return AM32RET_ERR_BADACK;
        }
    }
    return AM32RET_ERR_BADCRC;
}

static void am32_appendCrc(uint8_t* buffer, int cnt)
{
    uint16_t crc = am32_crc(buffer, cnt - 2);
    uint8_t* pcrc = (uint8_t*)&crc;
    buffer[cnt]     = pcrc[0];
    buffer[cnt + 1] = pcrc[1];
}

static uint16_t am32_crc(uint8_t* buffer, int cnt)
{
    uint16_t crc = 0;
    for (int i = 0; i < cnt; i++)
    {
        uint8_t xb = buffer[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (((xb & 0x01) ^ (crc & 0x0001)) !=0) {
                crc = crc >> 1;
                crc = crc ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
            xb = xb >> 1;
        }
    }
    return crc;
}

static int am32_cmdAddress(uint16_t addr)
{
    uint8_t buff[] = { 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00 };
    uint8_t* paddr = (uint8_t*)&addr;
    buff[2] = paddr[1];
    buff[3] = paddr[0];
    am32_appendCrc(buff, 4);
    ser->write((const char*)buff, 6);
    int r = am32_receive(buff, 1);
    if (r == 1) {
        if (buff[0] == AM32REPLY_ACK) {
            return AM32RET_SUCCESS;
        }
        else {
            return AM32RET_ERR_BADACK;
        }
    }
    else {
        return AM32RET_ERR_TIMEOUT;
    }
}

static int am32_cmdBufferLen(uint8_t datacnt)
{
    uint8_t buff[] = { 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00 };
    buff[3] = datacnt;
    am32_appendCrc(buff, 4);
    ser->write((const char*)buff, 6);
    return AM32RET_SUCCESS;
}

static int am32_cmdPayload(uint8_t* data, uint8_t datacnt)
{
    uint8_t buff[1];
    am32_appendCrc(data, datacnt);
    ser->write((const char*)data, datacnt + 2);
    int r = am32_receive(buff, 1);
    if (r == 1) {
        if (buff[0] == AM32REPLY_ACK) {
            return AM32RET_SUCCESS;
        }
        else {
            return AM32RET_ERR_BADACK;
        }
    }
    else {
        return AM32RET_ERR_TIMEOUT;
    }
}

static int am32_cmdFlash(void)
{
    uint8_t buff[] = { 0x01, 0x01, 0x00, 0x00 };
    am32_appendCrc(buff, 2);
    ser->write((const char*)buff, 4);
    int r = am32_receive(buff, 1);
    if (r == 1) {
        if (buff[0] == AM32REPLY_ACK) {
            return AM32RET_SUCCESS;
        }
        else {
            return AM32RET_ERR_BADACK;
        }
    }
    else {
        return AM32RET_ERR_TIMEOUT;
    }
}

static int am32_cmdRead(uint8_t* data, uint8_t datacnt)
{
    uint8_t buff[] = { 0x03, 0x00, 0x00, 0x00 };
    buff[1] = datacnt;
    am32_appendCrc(buff, 2);
    ser->write((const char*)buff, 4);
    int r = am32_receive(buff, datacnt + 3);
    if (r == datacnt + 3) {
        return am32_verify(data, r);
    }
    else {
        return AM32RET_ERR_TIMEOUT;
    }
}

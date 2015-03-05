/*
 * Copyright (c) 2012-2015, Mike Smith, <msmith@purgatory.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * o Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file ebl.cpp
 *
 * EBL protocol decoder.
 */

#include "EBLmon.h"
#include "board.h"

#include <math.h>

namespace EBL
{
bool        connected = false;

uint8_t     mem[256];
uint16_t    adc[8];

bool        updated = false;

enum {
    WAIT_H1,
    WAIT_H2,
    ARRAY,
    STATUS,
    ADC,
    WAIT_C1,
    WAIT_C2
}           state = WAIT_H1;
unsigned    bytes;
uint16_t    running_sum;

void
decode(uint8_t c)
{
    running_sum += c;

    switch (state) {
    case WAIT_H1:
        if (c == 0xaa) {
            state = WAIT_H2;
            running_sum = c;
        }

        break;

    case WAIT_H2:
        if (c == 0x55) {
            state = ARRAY;
            bytes = 0;

        } else {
            state = WAIT_H1;
        }

        break;

    case ARRAY:
        mem[bytes] = c;

        if (bytes++ == 256) {
            state = STATUS;
        }

        break;

    case STATUS:
        state = ADC;    // currently just discard this byte
        bytes = 0;
        break;

    case ADC:
        if ((bytes & 1) == 0) {
            // low byte
            adc[bytes / 2] = c;

        } else {
            adc[bytes / 2] += (uint16_t)c << 8;
        }

        if (bytes++ == 16) {
            state = WAIT_C1;
        }

        break;

    case WAIT_C1:
        // subtract once because we added it above, and once more because this
        // is the low byte of the expected checksum (should leave the low byte
        // equal to zero
        running_sum -= c;
        running_sum -= c;
        state = WAIT_C2;
        break;

    case WAIT_C2:
        // subtract once because we added it above
        running_sum -= c;

        // should be the high byte of the running checksum
        if (running_sum == (c << 8)) {
            updated = true;
        }

        state = WAIT_H1;
        break;

    default:
        state = WAIT_H1;
        break;
    }
}

bool
was_updated()
{
    if (updated) {
        updated = false;
        return true;
    }

    return false;
}

unsigned
engine_speed()
{
    // below 6375 rpm could use byte_1c * 25...
    return mem[0xf3] * 31U + mem[0xf3] / 4;
}

unsigned
ground_speed()
{
    return mem[0x34];
}

unsigned
oil_pressure()
{
    // 10-bit ADC reading 0-5V
    // 100psi sensor over the range 0.5-4V
    // 0.5V = 102.4 counts
    // 4.5V = 921.6 counts
    // span is 819.2 counts, conversion is / 8.192

    unsigned counts = adc[2];

    // XXX should record a local DTC for out-of-bounds values?
    if (counts > 102) {
        counts -= 102;
    } else if (counts > 921) {
        counts = 921;
    }

    float pressure = counts / 8.192F;

    return roundf(pressure);
}

unsigned
water_temperature()
{
    float temperature = mem[0xe3] * 0.75F - 40;

    if (temperature < 0) {
        return 0;
    }
    return roundf(temperature);
}

unsigned
voltage()
{
    return mem[0x45];
}

unsigned
afr()
{
    // 10-bit ADC reading 0-5V
    // Zeitronix AFR default output mode, AFR is 2 * voltage + 9.6
    // 0V = 9.6:1
    // 5V = 19.6:1
    // span is 1024 counts, conversion is / 102.4 + 9.6

    unsigned counts = adc[1];

    float ratio = counts / 102.4F + 9.6F;

    return roundf(ratio * 10);
}

bool
ses_set()
{
    return mem[0x0b] & 0x1;
}

bool
engine_running()
{
    return mem[0x01] & 0x80;
}

const char *
dtc_string(uint8_t index)
{
    // sort into priority order

    if ((mem[0x12] & 0x01) && (index-- == 0)) {
        return "VSS   ";
    }

    if ((mem[0x12] & 0x02) && (index-- == 0)) {
        return "IAT LO";
    }

    if ((mem[0x12] & 0x04) && (index-- == 0)) {
        return "TPS LO";
    }

    if ((mem[0x12] & 0x08) && (index-- == 0)) {
        return "TPS HI";
    }

    if ((mem[0x12] & 0x10) && (index-- == 0)) {
        return "CTS LO";
    }

    if ((mem[0x12] & 0x20) && (index-- == 0)) {
        return "CTS HI";
    }

    if ((mem[0x12] & 0x40) && (index-- == 0)) {
        return "O2    ";
    }

    if ((mem[0x12] & 0x80) && (index-- == 0)) {
        return "DRP   ";
    }

    if ((mem[0x13] & 0x01) && (index-- == 0)) {
        return "EST   ";
    }

    if ((mem[0x13] & 0x08) && (index-- == 0)) {
        return "MAP LO";
    }

    if ((mem[0x13] & 0x10) && (index-- == 0)) {
        return "MAP HI";
    }

    if ((mem[0x13] & 0x80) && (index-- == 0)) {
        return "IAT HI";
    }

    if ((mem[0x14] & 0x01) && (index-- == 0)) {
        return "ADU   ";
    }

    if ((mem[0x14] & 0x02) && (index-- == 0)) {
        return "FP RLY";
    }

    if ((mem[0x14] & 0x04) && (index-- == 0)) {
        return "VATS  ";
    }

    if ((mem[0x14] & 0x08) && (index-- == 0)) {
        return "CALPAK";
    }

    if ((mem[0x14] & 0x10) && (index-- == 0)) {
        return "PROM  ";
    }

    if ((mem[0x14] & 0x20) && (index-- == 0)) {
        return "O2 RH ";
    }

    if ((mem[0x14] & 0x40) && (index-- == 0)) {
        return "O2 LN ";
    }

    if ((mem[0x14] & 0x80) && (index-- == 0)) {
        return "ESC   ";
    }

    return nullptr;
}
} // namespace EBL


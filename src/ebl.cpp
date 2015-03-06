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

uint8_t     ebl_mem[256];
uint16_t    ebl_adc[8];

bool        updated = false;

volatile unsigned rx_count = 0;
volatile unsigned good_packets = 0;
volatile unsigned bad_packets = 0;

enum DecodeState {
    WAIT_H1,
    WAIT_H2,
    ARRAY,
    STATUS,
    ADC,
    WAIT_C1,
    WAIT_C2
};

void
decode(uint8_t c)
{
    static unsigned running_sum = 0U;
    static unsigned field_index = 0U;
    static DecodeState state = WAIT_H1;

    running_sum += c;
    rx_count++;

    switch (state) {
    case WAIT_H1:
        if (c == 0x55) {
            state = WAIT_H2;
            running_sum = c;
        }

        break;

    case WAIT_H2:
        if (c == 0xaa) {
            state = ARRAY;
            field_index = 0;

        } else {
            state = WAIT_H1;
        }

        break;

    case ARRAY:
        ebl_mem[field_index] = c;

        if (++field_index == 256) {
            state = STATUS;
        }

        break;

    case STATUS:
        state = ADC;    // currently just discard this byte
        field_index = 0;
        break;

    case ADC:
        if ((field_index & 1) == 0) {
            ebl_adc[field_index / 2] = c;

        } else {
            ebl_adc[field_index / 2] += (uint16_t)c << 8;
        }

        if (++field_index == 16) {
            state = WAIT_C1;
        }

        break;

    case WAIT_C1:
        // subtract this byte from the running sum, since it shouldn't be included
        running_sum -= c;

        // this is the high byte of the running sum, so subtract it out
        running_sum -= (c << 8);
        state = WAIT_C2;
        break;

    case WAIT_C2:
        // subtract this byte from the running sum, since it shouldn't be included
        running_sum -= c;

        // ths is the low byte of the running sum, so should be equal
        if (running_sum == c) {
            //updated = true;
            good_packets++;

        } else {
            bad_packets++;
            debug("bad sum %04x", running_sum);
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
    return ebl_mem[0xf3] * 31U + ebl_mem[0xf3] / 4;
}

unsigned
ground_speed()
{
    return ebl_mem[0x34];
}

unsigned
oil_pressure()
{
    // 10-bit ADC reading 0-5V
    // 100psi sensor over the range 0.5-4V
    // 0.5V = 102.4 counts
    // 4.5V = 921.6 counts
    // span is 819.2 counts, conversion is / 8.192

    unsigned counts = ebl_adc[2];

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
    float temperature = ebl_mem[0xe3] * 0.75F - 40;

    if (temperature < 0) {
        return 0;
    }

    return roundf(temperature);
}

unsigned
voltage()
{
    return ebl_mem[0x45];
}

unsigned
afr()
{
    // 10-bit ADC reading 0-5V
    // Zeitronix AFR default output mode, AFR is 2 * voltage + 9.6
    // 0V = 9.6:1
    // 5V = 19.6:1
    // span is 1024 counts, conversion is / 102.4 + 9.6

    unsigned counts = ebl_adc[1];

    float ratio = counts / 102.4F + 9.6F;

    return roundf(ratio * 10);
}

bool
ses_set()
{
    return ebl_mem[0x0b] & 0x1;
}

bool
engine_running()
{
    return ebl_mem[0x01] & 0x80;
}

const char *
dtc_string(uint8_t dtc_index)
{
    // sort into priority order

    if ((ebl_mem[0x12] & 0x01) && (dtc_index-- == 0)) {
        return "VSS   ";
    }

    if ((ebl_mem[0x12] & 0x02) && (dtc_index-- == 0)) {
        return "IAT LO";
    }

    if ((ebl_mem[0x12] & 0x04) && (dtc_index-- == 0)) {
        return "TPS LO";
    }

    if ((ebl_mem[0x12] & 0x08) && (dtc_index-- == 0)) {
        return "TPS HI";
    }

    if ((ebl_mem[0x12] & 0x10) && (dtc_index-- == 0)) {
        return "CTS LO";
    }

    if ((ebl_mem[0x12] & 0x20) && (dtc_index-- == 0)) {
        return "CTS HI";
    }

    if ((ebl_mem[0x12] & 0x40) && (dtc_index-- == 0)) {
        return "O2    ";
    }

    if ((ebl_mem[0x12] & 0x80) && (dtc_index-- == 0)) {
        return "DRP   ";
    }

    if ((ebl_mem[0x13] & 0x01) && (dtc_index-- == 0)) {
        return "EST   ";
    }

    if ((ebl_mem[0x13] & 0x08) && (dtc_index-- == 0)) {
        return "MAP LO";
    }

    if ((ebl_mem[0x13] & 0x10) && (dtc_index-- == 0)) {
        return "MAP HI";
    }

    if ((ebl_mem[0x13] & 0x80) && (dtc_index-- == 0)) {
        return "IAT HI";
    }

    if ((ebl_mem[0x14] & 0x01) && (dtc_index-- == 0)) {
        return "ADU   ";
    }

    if ((ebl_mem[0x14] & 0x02) && (dtc_index-- == 0)) {
        return "FP RLY";
    }

    if ((ebl_mem[0x14] & 0x04) && (dtc_index-- == 0)) {
        return "VATS  ";
    }

    if ((ebl_mem[0x14] & 0x08) && (dtc_index-- == 0)) {
        return "CALPAK";
    }

    if ((ebl_mem[0x14] & 0x10) && (dtc_index-- == 0)) {
        return "PROM  ";
    }

    if ((ebl_mem[0x14] & 0x20) && (dtc_index-- == 0)) {
        return "O2 RH ";
    }

    if ((ebl_mem[0x14] & 0x40) && (dtc_index-- == 0)) {
        return "O2 LN ";
    }

    if ((ebl_mem[0x14] & 0x80) && (dtc_index-- == 0)) {
        return "ESC   ";
    }

    return nullptr;
}
} // namespace EBL


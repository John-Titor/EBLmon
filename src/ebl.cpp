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

namespace EBL
{
bool        connected = false;

uint8_t     byte_0b;
uint8_t     byte_12;
uint8_t     byte_13;
uint8_t     byte_14;
uint8_t     byte_1c;
uint8_t     byte_34;
uint8_t     byte_45;
uint8_t     byte_e3;
uint8_t     byte_f3;

uint16_t    adc[3];

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
uint8_t     check_1;

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
        switch (bytes) {
        case 0x0b:
            byte_0b = c;
            break;

        case 0x12:
            byte_12 = c;
            break;

        case 0x13:
            byte_13 = c;
            break;

        case 0x14:
            byte_14 = c;
            break;

        case 0x1c:
            byte_1c = c;
            break;

        case 0x34:
            byte_34 = c;
            break;

        case 0x45:
            byte_45 = c;
            break;

        case 0xe3:
            byte_e3 = c;
            break;

        case 0xf3:
            byte_f3 = c;
            break;

        default:
            break;
        }

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
        check_1 = c;
        state = WAIT_C2;
        break;

    case WAIT_C2:
        running_sum -= (check_1 + c);

        if (((running_sum % 0xff) == check_1) &&
            ((running_sum >> 8) == c)) {
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
    return byte_f3 * 31U + byte_f3 / 4;
}

unsigned
ground_speed()
{
    return byte_34;
}

float
oil_pressure()
{
    // 10-bit ADC reading 0-5V
    // 100psi sensor over the range 0.5-4V
    // 0.5V = 102.4 counts
    // 4.5V = 921.6 counts
    // span is 819.2 counts, conversion is / 8.192

    float pressure = adc[2] - 102.4F;

    if (pressure < 0) {
        pressure = 0;
    }

    pressure /= 8.192F;

    return pressure;
}

float
water_temperature()
{
    return byte_e3 * 0.75F - 40;
}

float
voltage()
{
    return byte_45 / 10.0F;
}

bool
ses_set()
{
    return byte_0b & 0x1;
}

const char *
status()
{

}

const char *
dtc_string(uint8_t index)
{
    // sort into priority order

    if ((byte_12 & 0x01) && (index-- == 0)) {
        return "VSS   ";
    }

    if ((byte_12 & 0x02) && (index-- == 0)) {
        return "IAT LO";
    }

    if ((byte_12 & 0x04) && (index-- == 0)) {
        return "TPS LO";
    }

    if ((byte_12 & 0x08) && (index-- == 0)) {
        return "TPS HI";
    }

    if ((byte_12 & 0x10) && (index-- == 0)) {
        return "CTS LO";
    }

    if ((byte_12 & 0x20) && (index-- == 0)) {
        return "CTS HI";
    }

    if ((byte_12 & 0x40) && (index-- == 0)) {
        return "O2    ";
    }

    if ((byte_12 & 0x80) && (index-- == 0)) {
        return "DRP   ";
    }

    if ((byte_13 & 0x01) && (index-- == 0)) {
        return "EST   ";
    }

    if ((byte_13 & 0x08) && (index-- == 0)) {
        return "MAP LO";
    }

    if ((byte_13 & 0x10) && (index-- == 0)) {
        return "MAP HI";
    }

    if ((byte_13 & 0x80) && (index-- == 0)) {
        return "IAT HI";
    }

    if ((byte_14 & 0x01) && (index-- == 0)) {
        return "ADU   ";
    }

    if ((byte_14 & 0x02) && (index-- == 0)) {
        return "FP RLY";
    }

    if ((byte_14 & 0x04) && (index-- == 0)) {
        return "VATS  ";
    }

    if ((byte_14 & 0x08) && (index-- == 0)) {
        return "CALPAK";
    }

    if ((byte_14 & 0x10) && (index-- == 0)) {
        return "PROM  ";
    }

    if ((byte_14 & 0x20) && (index-- == 0)) {
        return "O2 RH ";
    }

    if ((byte_14 & 0x40) && (index-- == 0)) {
        return "O2 LN ";
    }

    if ((byte_14 & 0x80) && (index-- == 0)) {
        return "ESC   ";
    }

    return nullptr;
}
} // namespace EBL


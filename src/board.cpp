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
 * @file board.cpp
 *
 * Generic board-support code.
 */

#include "board.h"

/****************************************************************************
 * Serial port
 */

void Board::com_init(unsigned speed __unused) {}

uint8_t
Board::com_getc(void)
{
    TCritSect cs;

    while (_rx_tail == _rx_head) {
        _rx_data_avail.wait();
    }

    auto c = _rx_buf[_rx_head];
    _rx_head = (_rx_head + 1) % _rx_buf_size;

    return c;
}

void
Board::com_rx(uint8_t c)
{
    auto next = (_rx_tail + 1) % _rx_buf_size;

    // rx buffer overflow, drop oldest byte
    if (next == _rx_head) {
        return;
    }

    _rx_buf[_rx_tail] = c;
    _rx_tail = next;

    // wake anyone that might be waiting
    _rx_data_avail.signal_isr();
}

void Board::led_set(bool state __unused) {}
void Board::led_toggle() {}

/****************************************************************************
 * u8g graphics library support
 */
extern "C" void
u8g_Delay(uint16_t val)
{
    OS::sleep(val);
}

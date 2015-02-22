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
void Board::com_fini() {}

void
Board::com_write(const uint8_t *data, unsigned count)
{
    TCritSect cs;

    while (count) {
        auto avail = _com_tx_buf.get_free_size();

        /* if we have no tx space, wait for some to free up */
        if (avail == 0) {
            _tx_space_avail.wait();
            continue;
        }

        if (avail > count)
            avail = count;

        _com_tx_buf.write(data, avail);
        data += avail;
        count -= avail;

        com_tx_start();
    }
}

int
Board::com_read(uint8_t *data, unsigned size, bool wait)
{
    TCritSect cs;

    for (;;) {
        unsigned avail = _com_rx_buf.get_count();

        if (avail != 0) {
            if (avail > size)
                avail = size;

            _com_rx_buf.read(data, avail);
            return avail;
        }

        if (wait == false) {
            return 0;
        }

        _rx_data_avail.wait();
    }
}

int
Board::com_write_space()
{
    return _com_tx_buf.get_free_size();
}

int
Board::com_read_available()
{
    return _com_rx_buf.get_count();
}

void
Board::com_rx(uint8_t c)
{
    _com_rx_buf.push_back(c);

    /* wake anyone that might be waiting */
    _rx_data_avail.signal_isr();
}

bool
Board::com_tx(uint8_t &c)
{
    auto avail = _com_tx_buf.get_count();

    /* if there is no more data to send, bail now */
    if (avail == 0)
        return false;

    /*
     * Mitigate writer wakeup costs by only signalling that there is
     * more TX space when at least 8 bytes are free.
     */
    if ((_com_tx_buf.get_free_size() - avail) > 8)
        _tx_space_avail.signal_isr();

    /*
     * Get the byte we're going to send.
     */
    c = _com_tx_buf.pop_front();
    return true;
}

void Board::com_tx_start(void) {}

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

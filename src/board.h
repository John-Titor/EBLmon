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
 * @file board.h
 *
 * Board interface definitions.
 */

#pragma once

#include <u8g.h>
#include <m2.h>
#include <usrlib.h>

#include "EBLmon.h"

class Board;
extern Board *gBoard;

/*
 * The board support code must export the m2 event source.
 */
extern uint8_t m2_board_es(m2_p ep, uint8_t msg);

/**
 * Abstract class that board drivers inherit.
 *
 * Board drivers are responsible for:
 * - Board / SoC init
 * - Serial Rx/Tx
 * - LED
 * - Display init & output
 */
class Board
{
public:
    Board(u8g_dev_t *with_u8g_dev) :
        _u8g_dev(with_u8g_dev)
    {
    }

    /**
     * Configure the serial port.
     *
     * @param speed             The port speed in bits per second.
     */
    virtual void                com_init(unsigned speed);

    /**
     * Read data from the serial port.
     *
     * This function will block until data is available.
     *
     * @return                  Data read from the port.
     */
    uint8_t                     com_getc(void);

    /**
     * Turn the LED on or off.
     *
     * @param state             The new LED state (true = on)
     */
    virtual void                led_set(bool state);

    /**
     * Toggle the LED.
     */
    virtual void                led_toggle();

    /**
     * Fetch the graphics device driver.
     */
    u8g_dev_t                   *u8g_dev() { return _u8g_dev; }

    unsigned                    com_interrupts = 0;

protected:
    /** graphics driver */
    u8g_dev_t                   *_u8g_dev;

    /**
     * Called by the board-specific subclass to add a byte
     * to the receive buffer.
     *
     * @param c                 The received byte.
     */
    void                        com_rx(uint8_t c);

private:
    static const unsigned       _rx_buf_size = 1024;
    OS::TEventFlag              _rx_data_avail;
    uint8_t                     _rx_buf[_rx_buf_size + 1];
    unsigned                    _rx_head = 0;
    unsigned                    _rx_tail = 0;
};

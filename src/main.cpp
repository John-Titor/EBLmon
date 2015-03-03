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
 * @file main.cpp
 *
 * Application startup logic.
 */

#include "board.h"

typedef OS::process<OS::pr0, 1000> TGUIProc;
typedef OS::process<OS::pr1, 1000> TCommsProc;
typedef OS::process<OS::pr2, 1000> TLEDProc;

TGUIProc GUIProc;
TCommsProc CommsProc;
TLEDProc LEDProc;

OS::TEventFlag msTick;

extern "C" void
main()
{
    // condfigure the board
    gBoard->led_set(true);
    gBoard->com_init(57600);

    // and start the OS
    OS::run();
}

namespace OS
{

// User interface process, run the LCD and controls
template <>
OS_PROCESS void TGUIProc::exec()
{
    UI::init();

    for (;;) {
        UI::tick();
        OS::sleep(10);
    }
}

// Comms process, handle incoming data from the ECU
template <>
OS_PROCESS void TCommsProc::exec()
{
    for (;;) {
        uint8_t c;

        // block waiting for data
        gBoard->com_read(&c, 1, true);

        // run the decode state machine
        EBL::decode(c);
    }
}

// Heartbeat process
template <>
OS_PROCESS void TLEDProc::exec()
{
    for (;;) {
        OS::sleep(125);
        gBoard->led_toggle();
    }
}
}

void OS::system_timer_user_hook()
{
    msTick.signal_isr();
}

#if scmRTOS_IDLE_HOOK_ENABLE
void OS::idle_process_user_hook()
{
    __WFI();
}
#endif

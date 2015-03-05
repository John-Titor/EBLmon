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
 * @file EBLmon.h
 *
 * Global definitions for EBLmon.
 */

#pragma once

#include <stdio.h>
#include <scmRTOS.h>

#define debug(fmt, args...)	do { printf(fmt "\r\n", ##args); } while(0)

#define __unused	__attribute__((unused))
#define __noreturn	__attribute__((noreturn))

extern OS::TEventFlag msTick;

namespace UI
{
extern void init();
extern void tick();
}

namespace EBL
{
extern void decode(uint8_t c);
extern bool was_updated();
extern unsigned engine_speed();		// rpm
extern unsigned ground_speed();		// mph
extern unsigned oil_pressure();		// psi
extern unsigned water_temperature();	// degrees C
extern unsigned voltage();		// decivolts
extern unsigned afr();			// afr * 10
extern bool ses_set();
extern bool engine_running();
extern const char *status();
extern const char *dtc_string(uint8_t index);
}

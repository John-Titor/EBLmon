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
 * @file ui_menu.c
 *
 * Menu tree for MAVmon.
 *
 * Font usage:
 * 0 - normal text (default)
 * 1 - titles
 * 2 - m2 glyphs (should already be set up?)
 */

#include <u8g.h>
#include <m2.h>
#include <stdio.h>

#pragma GCC diagnostic ignored "-Wold-style-declaration"
const M2_EXTERN_ALIGN(ui_status);
#pragma GCC diagnostic warning "-Wold-style-declaration"

/* fonts */
const void *const ui_fonts[] = {
    u8g_font_6x13B,             // f0 - medium text
    u8g_font_profont22,         // f1 - large display fields
    NULL
};

/*
 * Top-level settings menu
 */
static const M2_LABEL(_settings_title, "f1", "Settings");
static const M2_ROOT(_settings_status, "f0", "DONE", &ui_status);
static const M2_LIST(_settings_list) = {
    &_settings_title,
    &_settings_status
};
static const M2_VLIST(_settings_vlist, NULL, _settings_list);
static const M2_ALIGN(_settings, "-0|2W64H63", &_settings_vlist);

/*
 * Status display - two large strings and a small one.
 */
const char *ui_coolant_temperature = "100\xb0";
const char *ui_oil_pressure = "50#";
const char *ui_battery_voltage = "12.9v";
const char *ui_afr = "11.9";
const char *ui_status_text = "OK                   ";

static const M2_LABELPTR(_status_coolant_temp_, "f1", &ui_coolant_temperature);
static const M2_ALIGN(_status_coolant_temp, "x0y40w64h24", &_status_coolant_temp_);

static const M2_LABELPTR(_status_oil_pressure_, "f1", &ui_oil_pressure);
static const M2_ALIGN(_status_oil_pressure, "x64y40w64h24", &_status_oil_pressure_);

static const M2_LABELPTR(_status_voltage_, "f1", &ui_battery_voltage);
static const M2_ALIGN(_status_voltage, "x0y14w64h24", &_status_voltage_);

static const M2_LABELPTR(_status_afr_, "f1", &ui_afr);
static const M2_ALIGN(_status_afr, "x64y14w64h24", &_status_afr_);

static void _go_settings(m2_el_fnarg_p fnarg) { m2_SetRoot(&_settings); }
static const M2_BUTTONPTR(_status_settings,     "x0y0f0", &ui_status_text, &_go_settings);

static const M2_LIST(_status_list) = {
    &_status_coolant_temp,
    &_status_oil_pressure,
    &_status_voltage,
    &_status_afr,
    &_status_settings
};

static const M2_XYLIST(_status_vlist, NULL, _status_list);
const M2_ALIGN(ui_status, "-0|2W64H63", &_status_vlist);


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
 * @file ui.cpp
 *
 * User interface for EBLmon.
 */


#include "EBLmon.h"
#include "board.h"

#include <u8g.h>
#include <m2.h>
//#include <m2utl.h> /* doesn't seem necessary yet */
#include <m2ghu8g.h>



namespace UI
{
// graphics driver
u8g_t u8g;

// root of the UI tree
M2_EXTERN_ALIGN(ui_status);

// fonts
const void *const ui_fonts[] = {
    u8g_font_6x13B,             // f0 - medium text
    u8g_font_profont22,         // f1 - large display fields
    NULL
};

void
init()
{
    /* graphics driver init */
    u8g_Init(&u8g, gBoard->u8g_dev());

    /* m2tk init */
    m2_Init(&ui_status,         /* UI root */
            m2_board_es,        /* event source */
            m2_eh_4bd,          /* event handler */
            m2_gh_u8g_bfs);     /* UI style */
    m2_SetU8g(&u8g, m2_u8g_box_icon);

    for (unsigned i = 0; ui_fonts[i] != NULL; i++) {
        m2_SetFont(i, ui_fonts[i]);
    }
}

void
tick()
{
    m2_CheckKey();

    if (m2_HandleKey()) {
        /* picture loop */
        u8g_FirstPage(&u8g);

        do {
            m2_Draw();
            m2_CheckKey();
        } while (u8g_NextPage(&u8g));
    }
}

// Top-level settings menu
// 
static M2_LABEL(_settings_title, "f1", "Settings");
static M2_ROOT(_settings_status, "f0", "DONE", &ui_status);
static M2_LIST(_settings_list) = {
    &_settings_title,
    &_settings_status
};
static M2_VLIST(_settings_vlist, NULL, _settings_list);
static M2_ALIGN(_settings, "-0|2W64H63", &_settings_vlist);

// Status display - Four-quadrant display plus status bar
//
const char *ui_coolant_temperature = "100\xb0";
const char *ui_oil_pressure = "50#";
const char *ui_battery_voltage = "12.9v";
const char *ui_afr = "11.9";
const char *ui_status_text = "OK                   ";

static M2_LABELPTR(_status_coolant_temp_, "f1", &ui_coolant_temperature);
static M2_ALIGN(_status_coolant_temp, "x0y40w64h24", &_status_coolant_temp_);

static M2_LABELPTR(_status_oil_pressure_, "f1", &ui_oil_pressure);
static M2_ALIGN(_status_oil_pressure, "x64y40w64h24", &_status_oil_pressure_);

static M2_LABELPTR(_status_voltage_, "f1", &ui_battery_voltage);
static M2_ALIGN(_status_voltage, "x0y14w64h24", &_status_voltage_);

static M2_LABELPTR(_status_afr_, "f1", &ui_afr);
static M2_ALIGN(_status_afr, "x64y14w64h24", &_status_afr_);

static void _go_settings(m2_el_fnarg_p fnarg) { m2_SetRoot(&_settings); }
static M2_BUTTONPTR(_status_settings,     "x0y0f0", &ui_status_text, &_go_settings);

static M2_LIST(_status_list) = {
    &_status_coolant_temp,
    &_status_oil_pressure,
    &_status_voltage,
    &_status_afr,
    &_status_settings
};

static M2_XYLIST(_status_vlist, NULL, _status_list);
M2_ALIGN(ui_status, "-0|2W64H63", &_status_vlist);

} // namespace UI

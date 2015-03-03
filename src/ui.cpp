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

char coolant_temperature[8] = {'-', '\0'};
char oil_pressure[8] = {'-', '\0'};
char battery_voltage[8] = {'-', '\0'};
char air_fuel_ratio[8] = {'-', '\0'};
char ebl_status[22] = {'N', 'O', 'T', ' ', 'C', 'O', 'N', 'N', 'E', 'C', 'T', 'E', 'D', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '\0'};

void
init()
{
    // graphics driver init
    u8g_Init(&u8g, gBoard->u8g_dev());

    // m2tk init
    m2_Init(&ui_status,         // UI root
            m2_board_es,        // event source
            m2_eh_4bd,          // event handler
            m2_gh_u8g_bfs);     // UI style
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

    if (EBL::was_updated()) {

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
const char *ui_cell_1_text = &coolant_temperature[0];
const char *ui_cell_2_text = &oil_pressure[0];
const char *ui_cell_3_text = &battery_voltage[0];
const char *ui_cell_4_text = &air_fuel_ratio[0];
const char *ui_status_text = &ebl_status[0];

static M2_LABELPTR(_status_cell_1_, "f1", &ui_cell_1_text);
static M2_ALIGN(_status_cell_1, "x0y40w64h24", &_status_cell_1_);

static M2_LABELPTR(_status_cell_2_, "f1", &ui_cell_2_text);
static M2_ALIGN(_status_cell_2, "x64y40w64h24", &_status_cell_2_);

static M2_LABELPTR(_status_cell_3_, "f1", &ui_cell_3_text);
static M2_ALIGN(_status_cell_3, "x0y14w64h24", &_status_cell_3_);

static M2_LABELPTR(_status_cell_4_, "f1", &ui_cell_4_text);
static M2_ALIGN(_status_cell_4, "x64y14w64h24", &_status_cell_4_);

static void _go_settings(m2_el_fnarg_p fnarg) { m2_SetRoot(&_settings); }
static M2_BUTTONPTR(_status_settings,     "x0y0f0", &ui_status_text, &_go_settings);

static M2_LIST(_status_list) = {
    &_status_cell_1,
    &_status_cell_2,
    &_status_cell_3,
    &_status_cell_4,
    &_status_settings
};

static M2_XYLIST(_status_vlist, NULL, _status_list);
M2_ALIGN(ui_status, "-0|2W64H63", &_status_vlist);

} // namespace UI

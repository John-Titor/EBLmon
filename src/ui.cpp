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
M2_EXTERN_ALIGN(ui_gauges);

// fonts
const void *const ui_fonts[] = {
    u8g_font_6x13B,             // f0 - medium text
    u8g_font_profont22,         // f1 - large display fields
    NULL
};

char road_speed[8] = {'-', '\0'};
char engine_speed[8] = {'-', '\0'};
char water_temperature[8] = {'-', '\0'};
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
    m2_Init(&ui_gauges,         // UI root
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

        sprintf(road_speed, "%umph", EBL::ground_speed());
        sprintf(engine_speed, "%urpm", EBL::engine_speed());
        sprintf(water_temperature, "%u\xb0", EBL::water_temperature());
        sprintf(oil_pressure, "%u#", EBL::oil_pressure());
        sprintf(battery_voltage, "%u.%uv", EBL::voltage() / 10, EBL::voltage() % 10);
        sprintf(air_fuel_ratio, "%u.%u", EBL::afr() / 10, EBL::afr() % 10);

        if (EBL::ses_set()) {
            sprintf(ebl_status, "CHECK ENGINE [%s]", EBL::dtc_string(0) ? : "??????");

        } else if (EBL::engine_running()) {
            sprintf(ebl_status, "OK                   ");

        } else {
            sprintf(ebl_status, "NOT RUNNING          ");
        }

    }
}


M2_EXTERN_ALIGN(_top);
M2_EXTERN_ALIGN(_settings);
M2_EXTERN_ALIGN(_stats);

const char *ui_status_text = &ebl_status[0];

// Top-level menu
//
M2_LABEL(_top_title, "f1", "Menu");
M2_ROOT(_top_settings, "f0", "Settings", &_settings);
M2_ROOT(_top_stats, "f0", "Stats", &_stats);
M2_ROOT(_top_done, "f0", "DONE", &ui_gauges);
M2_LIST(_top_list) = {
    &_top_title,
    &_top_settings,
    &_top_stats,
    &_top_done
};
M2_VLIST(_top_vlist, NULL, _top_list);
M2_ALIGN(_top, "-0|2W64H63", &_top_vlist);

// Settings menu
//
M2_LABEL(_settings_title, "f1", "Settings");
M2_ROOT(_settings_done, "f0", "DONE", &_top);
M2_LIST(_settings_list) = {
    &_settings_title,
    &_settings_done
};
M2_VLIST(_settings_vlist, NULL, _settings_list);
M2_ALIGN(_settings, "-0|2W64H63", &_settings_vlist);

// Stats menu
//
M2_U32NUM(_stats_pkts, "r1f0", (uint32_t *)&EBL::good_packets);
M2_U32NUM(_stats_badpkts, "r1f0", (uint32_t *)&EBL::bad_packets);
M2_ROOT(_stats_done, "f0", "DONE", &_top);
M2_LIST(_stats_list) = {
    &_stats_pkts,
    &_stats_badpkts,
    &_stats_done
};
M2_VLIST(_stats_vlist, NULL, _stats_list);
M2_ALIGN(_stats, "-0|2W64H63", &_stats_vlist);

// Mini-dashboard - speed on top, RPM below
//
const char *ui_2cell_1_text = &road_speed[0];
const char *ui_2cell_2_text = &engine_speed[0];

M2_LABELPTR(_dash_cell_1_, "f1", &ui_2cell_1_text);
M2_ALIGN(_dash_cell_1, "x0y40w128h24", &_dash_cell_1_);

M2_LABELPTR(_dash_cell_2_, "f1", &ui_2cell_2_text);
M2_ALIGN(_dash_cell_2, "x0y14w128h24", &_dash_cell_2_);

void _leave_dash(m2_el_fnarg_p fnarg) { m2_SetRootExtended(&_top, 2, 0); }
M2_BUTTONPTR(_dash_status,     "x0y0f0", &ui_status_text, &_leave_dash);

M2_LIST(_dash_list) = {
    &_dash_cell_1,
    &_dash_cell_2,
    &_dash_status
};

M2_XYLIST(_dash_vlist, NULL, _dash_list);
M2_ALIGN(_dash, "-0|2W64H63", &_dash_vlist);

// Status display - Four-quadrant display plus status bar
//
const char *_gauge_1_text = &water_temperature[0];
const char *_gauge_2_text = &oil_pressure[0];
const char *_gauge_3_text = &battery_voltage[0];
const char *_gauge_4_text = &air_fuel_ratio[0];

M2_LABELPTR(_gauges_cell_1_, "f1", &_gauge_1_text);
M2_ALIGN(_gauges_cell_1, "x0y40w64h24", &_gauges_cell_1_);

M2_LABELPTR(_gauges_cell_2_, "f1", &_gauge_2_text);
M2_ALIGN(_gauges_cell_2, "x64y40w64h24", &_gauges_cell_2_);

M2_LABELPTR(_gauges_cell_3_, "f1", &_gauge_3_text);
M2_ALIGN(_gauges_cell_3, "x0y14w64h24", &_gauges_cell_3_);

M2_LABELPTR(_gauges_cell_4_, "f1", &_gauge_4_text);
M2_ALIGN(_gauges_cell_4, "x64y14w64h24", &_gauges_cell_4_);

void _leave_gauges(m2_el_fnarg_p fnarg) { m2_SetRootExtended(&_dash, 0, 0); }
M2_BUTTONPTR(_gauges_status,     "x0y0f0", &ui_status_text, &_leave_gauges);

M2_LIST(_gauges_list) = {
    &_gauges_cell_1,
    &_gauges_cell_2,
    &_gauges_cell_3,
    &_gauges_cell_4,
    &_gauges_status
};

M2_XYLIST(_gauges_vlist, NULL, _gauges_list);
M2_ALIGN(ui_gauges, "-0|2W64H63", &_gauges_vlist);

} // namespace UI

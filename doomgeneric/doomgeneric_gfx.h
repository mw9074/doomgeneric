//
// Copyright(C) 2026 Michael Wilber
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//  doomgeneric graphics types and functions.
//
#ifndef DOOMGENERIC_GFX_H
#define DOOMGENERIC_GFX_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum dg_color_format {
    DG_COLOR_FORMAT_INVALID,
    DG_COLOR_FORMAT_RGB565,
    DG_COLOR_FORMAT_RGB888,
    DG_COLOR_FORMAT_RGBA8888,
} dg_color_format_t;

typedef struct dg_screen_info {
    uint32_t xres;
    uint32_t yres;
    dg_color_format_t color_format;
} dg_screen_info_t;

// DG_GraphicsLock() must be called before accessing this buffer and
// DG_GraphicsUnlock() must be called when the buffer is no longer in use.
extern uint8_t* doomgeneric_ScreenBuffer;

// The width parameter must be an even value.
void doomgeneric_SetScreenSize(uint32_t width, uint32_t height);

void doomgeneric_GetScreenInfo(dg_screen_info_t* info);

// Lock/Unlock the screen buffer and related variables.
// Must be implemented by external code, must be reentrant.
void DG_GraphicsLock();
void DG_GraphicsUnlock();

#ifdef __cplusplus
}
#endif

#endif // DOOMGENERIC_GFX_H

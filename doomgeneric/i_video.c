// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
// $Log:$
//
// DESCRIPTION:
//	DOOM graphics stuff for X11, UNIX.
//
//-----------------------------------------------------------------------------

static const char
rcsid[] = "$Id: i_x.c,v 1.6 1997/02/03 22:45:10 b1 Exp $";

#include "config.h"
#include "v_video.h"
#include "d_event.h"
#include "d_main.h"
#include "i_video.h"
#include "i_system.h"
#include "z_zone.h"

#include "tables.h"
#include "doomkeys.h"

#include "doomgeneric.h"
#include "doomgeneric_gfx.h"

#include <stdbool.h>
#include <stdlib.h>

#include <fcntl.h>

#include <stdarg.h>

#include <sys/types.h>

//#define CMAP256

struct FB_BitField
{
	uint32_t offset;			/* beginning of bitfield	*/
	uint32_t length;			/* length of bitfield		*/
};

struct FB_ScreenInfo
{
	uint32_t xres;			/* visible resolution		*/
	uint32_t yres;
	uint32_t xres_virtual;		/* virtual resolution		*/
	uint32_t yres_virtual;

	uint32_t bits_per_pixel;		/* guess what			*/
	
							/* >1 = FOURCC			*/
	struct FB_BitField red;		/* bitfield in s_Fb mem if true color, */
	struct FB_BitField green;	/* else only length is significant */
	struct FB_BitField blue;
	struct FB_BitField transp;	/* transparency			*/
};

static struct FB_ScreenInfo s_Fb;
static struct dg_screen_info s_DgScreenInfo;
int fb_scaling = 1;
int usemouse = 0;


#ifdef CMAP256

boolean palette_changed;
uint8_t colors[256];

#else  // CMAP256

// Current DOOM palette, converted to the current dg_color_format.
static uint8_t colors[256 * 4];

#endif  // CMAP256


void I_GetEvent(void);

// The screen buffer; this is modified to draw things to the screen

byte *I_VideoBuffer = NULL;

// The screen buffer read by external code.

size_t doomgeneric_ScreenBufferSize = 0;
uint8_t* doomgeneric_ScreenBuffer = NULL;

// If true, game is running as a screensaver

boolean screensaver_mode = false;

// Flag indicating whether the screen is currently visible:
// when the screen isnt visible, don't render the screen

boolean screenvisible;

// Mouse acceleration
//
// This emulates some of the behavior of DOS mouse drivers by increasing
// the speed when the mouse is moved fast.
//
// The mouse input values are input directly to the game, but when
// the values exceed the value of mouse_threshold, they are multiplied
// by mouse_acceleration to increase the speed.

float mouse_acceleration = 2.0;
int mouse_threshold = 10;

// Gamma correction level to use

int usegamma = 0;


static void cmap_to_fb(uint8_t* out, uint8_t* in, int in_pixels)
{
	int i, k;
	size_t bytes_per_pix = s_Fb.bits_per_pixel / 8;

	for (i = 0; i < in_pixels; i++, in++)
	{
		uint8_t* pix = colors + (((int)(*in)) * bytes_per_pix);

		for (k = 0; k < fb_scaling; k++)
		{
			memcpy(out, pix, bytes_per_pix);
			out += bytes_per_pix;
		}
	}
}

// Assumes DG_GraphicsLock() has been called
static void doomgeneric_EnsureScreenBufferSize(size_t requiredSize, bool needClear)
{
	if (requiredSize > doomgeneric_ScreenBufferSize)
	{
		needClear = true;

		free(doomgeneric_ScreenBuffer);
		doomgeneric_ScreenBufferSize = 0;

		doomgeneric_ScreenBuffer = malloc(requiredSize);
		if (doomgeneric_ScreenBuffer)
		{
			doomgeneric_ScreenBufferSize = requiredSize;
		}
	}

	if (needClear && doomgeneric_ScreenBuffer)
	{
		// Clear out the buffer so unused pixels draw as black.
		memset(doomgeneric_ScreenBuffer, 0, doomgeneric_ScreenBufferSize);
	}
}

void doomgeneric_GetScreenInfo(dg_screen_info_t* info)
{
	DG_GraphicsLock();

	*info = s_DgScreenInfo;

	DG_GraphicsUnlock();
}

void doomgeneric_SetInitialScreenInfo(const dg_screen_info_t* screen_info)
{
	DG_GraphicsLock();

	s_DgScreenInfo = *screen_info;

	DG_GraphicsUnlock();
}

void I_InitGraphics(void)
{
	int i;

	DG_GraphicsLock();

	memset(&s_Fb, 0, sizeof(struct FB_ScreenInfo));
	s_Fb.xres = s_DgScreenInfo.xres;
	s_Fb.yres = s_DgScreenInfo.yres;
	s_Fb.xres_virtual = s_Fb.xres;
	s_Fb.yres_virtual = s_Fb.yres;

#ifdef CMAP256

	s_Fb.bits_per_pixel = 8;

#else  // CMAP256

	if (DG_COLOR_FORMAT_RGBA8888 == s_DgScreenInfo.color_format) {
		s_Fb.bits_per_pixel = 32;

		s_Fb.blue.length = 8;
		s_Fb.green.length = 8;
		s_Fb.red.length = 8;
		s_Fb.transp.length = 8;

		s_Fb.blue.offset = 0;
		s_Fb.green.offset = 8;
		s_Fb.red.offset = 16;
		s_Fb.transp.offset = 24;
	}
	else if (DG_COLOR_FORMAT_RGB888 == s_DgScreenInfo.color_format) {
		s_Fb.bits_per_pixel = 24;

		s_Fb.blue.length = 8;
		s_Fb.green.length = 8;
		s_Fb.red.length = 8;
		s_Fb.transp.length = 0;

		s_Fb.blue.offset = 0;
		s_Fb.green.offset = 8;
		s_Fb.red.offset = 16;
		s_Fb.transp.offset = 0;
	}
	else if (DG_COLOR_FORMAT_RGB565 == s_DgScreenInfo.color_format) {
		s_Fb.bits_per_pixel = 16;

		s_Fb.blue.length = 5;
		s_Fb.green.length = 6;
		s_Fb.red.length = 5;
		s_Fb.transp.length = 0;

		s_Fb.blue.offset = 11;
		s_Fb.green.offset = 5;
		s_Fb.red.offset = 0;
		s_Fb.transp.offset = 16;
	}
	else
		I_Error("Unknown color format value: %d\n", (int)s_DgScreenInfo.color_format);


#endif  // CMAP256

	DG_Log("I_InitGraphics: framebuffer: x_res: %d, y_res: %d, x_virtual: %d, y_virtual: %d, bpp: %d\n",
		s_Fb.xres, s_Fb.yres, s_Fb.xres_virtual, s_Fb.yres_virtual, s_Fb.bits_per_pixel);

	DG_Log("I_InitGraphics: framebuffer: RGBA: %d%d%d%d, red_off: %d, green_off: %d, blue_off: %d, transp_off: %d\n",
		s_Fb.red.length, s_Fb.green.length, s_Fb.blue.length, s_Fb.transp.length, s_Fb.red.offset, s_Fb.green.offset, s_Fb.blue.offset, s_Fb.transp.offset);

	DG_Log("I_InitGraphics: DOOM screen size: w x h: %d x %d\n", SCREENWIDTH, SCREENHEIGHT);


	fb_scaling = s_Fb.xres / SCREENWIDTH;
	if (s_Fb.yres / SCREENHEIGHT < fb_scaling)
		fb_scaling = s_Fb.yres / SCREENHEIGHT;
	DG_Log("I_InitGraphics: Auto-scaling factor: %d\n", fb_scaling);


	/* Allocate screen to draw to */
	I_VideoBuffer = (byte*)Z_Malloc(SCREENWIDTH * SCREENHEIGHT, PU_STATIC, NULL);  // For DOOM to draw on

	/* Allocate the screen buffer read by external code. */
	size_t newSbSize = (size_t)(s_Fb.xres * s_Fb.yres * (s_Fb.bits_per_pixel / 8));
	doomgeneric_EnsureScreenBufferSize(newSbSize, true);

	screenvisible = true;
	DG_GraphicsUnlock();

	extern void I_InitInput(void);
	I_InitInput();
}

void doomgeneric_SetScreenSize(uint32_t width, uint32_t height)
{
	DG_GraphicsLock();

	bool size_changed = (width != s_DgScreenInfo.xres) || (height != s_DgScreenInfo.yres);

	if (size_changed)
	{
		s_DgScreenInfo.xres = width;
		s_DgScreenInfo.yres = height;

		s_Fb.xres = s_DgScreenInfo.xres;
		s_Fb.yres = s_DgScreenInfo.yres;
		s_Fb.xres_virtual = s_Fb.xres;
		s_Fb.yres_virtual = s_Fb.yres;

		int old_scaling = fb_scaling;

		fb_scaling = s_Fb.xres / SCREENWIDTH;
		if (s_Fb.yres / SCREENHEIGHT < fb_scaling)
			fb_scaling = s_Fb.yres / SCREENHEIGHT;
		if (fb_scaling != old_scaling)
		{
			DG_Log("I_InitGraphics: Auto-scaling factor: %d\n", fb_scaling);
		}
	}

	size_t newSbSize = (size_t)(s_Fb.xres * s_Fb.yres * (s_Fb.bits_per_pixel / 8));
	doomgeneric_EnsureScreenBufferSize(newSbSize, size_changed);

	DG_GraphicsUnlock();
}

void I_ShutdownGraphics(void)
{
	DG_GraphicsLock();

	Z_Free(I_VideoBuffer);

	free(doomgeneric_ScreenBuffer);
	doomgeneric_ScreenBuffer = NULL;
	doomgeneric_ScreenBufferSize = 0;

	DG_GraphicsUnlock();
}

void I_StartFrame (void)
{

}

void I_StartTic (void)
{
	I_GetEvent();
}

void I_UpdateNoBlit (void)
{
}

//
// I_FinishUpdate
//

void I_FinishUpdate (void)
{
    int y;
    int x_offset, y_offset, x_offset_end;
    unsigned char *line_in, *line_out;

    DG_GraphicsLock();

    /* Offsets in case FB is bigger than DOOM */
    /* 600 = s_Fb heigt, 200 screenheight */
    /* 600 = s_Fb heigt, 200 screenheight */
    /* 2048 =s_Fb width, 320 screenwidth */
    y_offset     = (((s_Fb.yres - (SCREENHEIGHT * fb_scaling)) * s_Fb.bits_per_pixel/8)) / 2;
    x_offset     = (((s_Fb.xres - (SCREENWIDTH  * fb_scaling)) * s_Fb.bits_per_pixel/8)) / 2; // XXX: siglent FB hack: /4 instead of /2, since it seems to handle the resolution in a funny way
    //x_offset     = 0;
    x_offset_end = ((s_Fb.xres - (SCREENWIDTH  * fb_scaling)) * s_Fb.bits_per_pixel/8) - x_offset;

    /* DRAW SCREEN */
    line_in  = (unsigned char *) I_VideoBuffer;
    line_out = (unsigned char *) doomgeneric_ScreenBuffer;

    y = SCREENHEIGHT;

    while (y--)
    {
        int i;
        for (i = 0; i < fb_scaling; i++) {
            line_out += x_offset;
#ifdef CMAP256
            if (fb_scaling == 1) {
                memcpy(line_out, line_in, SCREENWIDTH); /* fb_width is bigger than Doom SCREENWIDTH... */
            } else {
                int j;

                for (j = 0; j < SCREENWIDTH; j++) {
                    int k;
                    for (k = 0; k < fb_scaling; k++) {
                        line_out[j * fb_scaling + k] = line_in[j];
                    }
                }
            }
#else
            cmap_to_fb((void*)line_out, (void*)line_in, SCREENWIDTH);
#endif
            line_out += (SCREENWIDTH * fb_scaling * (s_Fb.bits_per_pixel/8)) + x_offset_end;
        }
        line_in += SCREENWIDTH;
    }

	DG_DrawFrame();
    DG_GraphicsUnlock();
}

//
// I_ReadScreen
//
void I_ReadScreen (byte* scr)
{
    memcpy (scr, I_VideoBuffer, SCREENWIDTH * SCREENHEIGHT);
}

//
// I_SetPalette
//
void I_SetPalette(byte* palette)
{
	int i;

	DG_GraphicsLock();



	if (DG_COLOR_FORMAT_RGB565 == s_DgScreenInfo.color_format)
	{
		// Convert the palette to RGB565 format for faster blitting later.
		uint16_t r, g, b, pix;
		byte* pal = palette;

		for (i = 0; i < 256; i++)
		{
			r = gammatable[usegamma][*pal++];
			g = gammatable[usegamma][*pal++];
			b = gammatable[usegamma][*pal++];

			pix = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

#ifdef SYS_BIG_ENDIAN
			pix = swapLE16(pix); // can't use SHORT() because this needs to stay unsigned
#endif
			memcpy(&colors[i * 2], &pix, 2);
		}
	}
	else if (DG_COLOR_FORMAT_RGB888 == s_DgScreenInfo.color_format ||
		DG_COLOR_FORMAT_RGBA8888 == s_DgScreenInfo.color_format)
	{
		// Convert the palette to RGB888 or RGBA8888 format for faster blitting later.
		uint32_t r, g, b, pix;
		uint8_t* c = colors;
		byte* pal = palette;

		int incr = (DG_COLOR_FORMAT_RGBA8888 == s_DgScreenInfo.color_format) ? 4 : 3;

		for (i = 0; i < 256; ++i) {
			r = ((uint32_t)(gammatable[usegamma][*pal++])) << s_Fb.red.offset;
			g = ((uint32_t)(gammatable[usegamma][*pal++])) << s_Fb.green.offset;
			b = ((uint32_t)(gammatable[usegamma][*pal++])) << s_Fb.blue.offset;
			pix = (r | g | b);

#ifdef SYS_BIG_ENDIAN
			pix = swapLE32(pix);
#endif
			memcpy(c, &pix, incr);
			c += incr;
		}
	}
	else
	{
		I_Error("Unknown color format value: %d\n", (int)s_DgScreenInfo.color_format);
	}

#ifdef CMAP256

	palette_changed = true;

#endif  // CMAP256

	DG_GraphicsUnlock();
}

void I_BeginRead (void)
{
}

void I_EndRead (void)
{
}

void I_SetWindowTitle (char *title)
{
	DG_SetWindowTitle(title);
}

void I_GraphicsCheckCommandLine (void)
{
}

void I_SetGrabMouseCallback (grabmouse_callback_t func)
{
}

void I_EnableLoadingDisk(void)
{
}

void I_BindVideoVariables (void)
{
}

void I_DisplayFPSDots (boolean dots_on)
{
}

void I_CheckIsScreensaver (void)
{
}

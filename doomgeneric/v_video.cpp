//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 1993-2008 Raven Software
// Copyright(C) 2005-2014 Simon Howard
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
//	Gamma correction LUT stuff.
//	Functions to draw patches (by post) directly to screen.
//	Functions to blit a block to the screen.
//

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "doomgeneric.h"
#include "i_system.h"

#include "doomtype.h"

#include "deh_str.h"
#include "i_swap.h"
#include "i_video.h"
#include "m_bbox.h"
#include "m_misc.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"

#include "config.h"
#ifdef HAVE_LIBPNG
#include <png.h>
#endif

// TODO: There are separate RANGECHECK defines for different games, but this
// is common code. Fix this.
#define RANGECHECK

enum class DrawMode
{
    Normal,
    Flipped,
    Shadowed,
    TL,
    XLA,
};

// Blending table used for fuzzpatch, etc.
// Only used in Heretic/Hexen

byte *tinttable = NULL;

// villsa [STRIFE] Blending table used for Strife
byte *xlatab = NULL;

// The screen buffer that the v_video.c code draws to.

static byte *dest_screen = NULL;


template <DrawMode drawMode>
void V_DrawPatchT(int x, int y, patch_t* patch);


//
// V_CopyRect
//
void V_CopyRect(int srcx, int srcy, byte *source,
                int width, int height,
                int destx, int desty)
{
    byte *src;
    byte *dest;

#ifdef RANGECHECK
    if (srcx < 0
     || srcx + width > SCREENWIDTH
     || srcy < 0
     || srcy + height > SCREENHEIGHT
     || destx < 0
     || destx + width > SCREENWIDTH
     || desty < 0
     || desty + height > SCREENHEIGHT)
    {
        I_Error ("Bad V_CopyRect");
    }
#endif

    src = source + SCREENWIDTH * srcy + srcx;
    dest = dest_screen + SCREENWIDTH * desty + destx;

    for ( ; height>0 ; height--)
    {
        memcpy(dest, src, width);
        src += SCREENWIDTH;
        dest += SCREENWIDTH;
    }
}


//
// V_DrawPatch
// Masks a column based masked pic to the screen.
//

void V_DrawPatch(int x, int y, patch_t *patch)
{
	V_DrawPatchT<DrawMode::Normal>(x, y, patch);
}

//
// V_DrawPatchFlipped
// Masks a column based masked pic to the screen.
// Flips horizontally, e.g. to mirror face.
//

void V_DrawPatchFlipped(int x, int y, patch_t *patch)
{
	V_DrawPatchT<DrawMode::Flipped>(x, y, patch);
}


//
// V_DrawPatchDirect
// Draws directly to the screen on the pc.
//

void V_DrawPatchDirect(int x, int y, patch_t* patch)
{
    V_DrawPatch(x, y, patch);
}

//
// V_DrawTLPatch
//
// Masks a column based translucent masked pic to the screen.
//

void V_DrawTLPatch(int x, int y, patch_t* patch)
{
    V_DrawPatchT<DrawMode::TL>(x, y, patch);
}

//
// V_DrawXlaPatch
//
// villsa [STRIFE] Masks a column based translucent masked pic to the screen.
//

void V_DrawXlaPatch(int x, int y, patch_t* patch)
{
    V_DrawPatchT<DrawMode::XLA>(x, y, patch);
}

//
// V_DrawShadowedPatch
//
// Masks a column based masked pic to the screen.
//

void V_DrawShadowedPatch(int x, int y, patch_t* patch)
{
    V_DrawPatchT<DrawMode::Shadowed>(x, y, patch);
}

template <DrawMode drawMode>
void V_DrawPatchT(int x, int y, patch_t* patch)
{
    int count;
    int colidx, col, row;
    column_t* column;
    byte* desttop, * dest;
    byte* desttop2, * dest2;
    byte* source;
    int w, h;
    int xadj = 0;
    int yadj = 0;

	(void)desttop2;
	(void)dest2;

    y -= SHORT(patch->topoffset);
    x -= SHORT(patch->leftoffset);

    if (x < 0)
    {
        xadj = -x;
        x = 0;
    }
    if (y < 0)
    {
        yadj = -y;
        y = 0;
    }

    desttop = dest_screen + (y * SCREENWIDTH) + x;
    if constexpr (drawMode == DrawMode::Shadowed)
    {
        desttop2 = dest_screen + ((y + 2) * SCREENWIDTH) + x + 2;
	}

    w = (int)SHORT(patch->width) - xadj;
    if (x + w > SCREENWIDTH)
    {
        w = SCREENWIDTH - x;
    }

    h = (int)SHORT(patch->height) - yadj;
    if (y + h > SCREENHEIGHT)
    {
        h = SCREENHEIGHT - y;
    }

    for (col = xadj; col < w; col++, desttop++)
    {
        if constexpr (drawMode == DrawMode::Flipped)
        {
            colidx = w - 1 - col;
        }
        else
        {
			colidx = col;
        }
        column = (column_t*)((byte*)patch + LONG(patch->columnofs[colidx]));

        // step through the posts in a column
        while (column->topdelta != 0xff)
        {
            source = (byte*)column + 3;
            dest = desttop + column->topdelta * SCREENWIDTH;
            if constexpr (drawMode == DrawMode::Shadowed)
            {
                dest2 = desttop2 + column->topdelta * SCREENWIDTH;
            }
            count = column->length;

            row = yadj;
            dest += row * SCREENWIDTH;

            for (; count && row < h; row++, count--)
            {
                if constexpr (drawMode == DrawMode::TL)
                {
                    *dest = tinttable[((*dest) << 8) + *source++];
                }
                else if constexpr (drawMode == DrawMode::XLA)
                {
                    *dest = xlatab[*dest + ((*source) << 8)];
				}
                else
                {
                    *dest = *source++;
                }
                dest += SCREENWIDTH;

                if constexpr (drawMode == DrawMode::Shadowed)
                {
                    *dest2 = tinttable[((*dest2) << 8)];
                    dest2 += SCREENWIDTH;
				}
            }
            column = (column_t*)((byte*)column + column->length + 4);
        }
    }
}

//
// Load tint table from TINTTAB lump.
//

void V_LoadTintTable(void)
{
    tinttable = static_cast<byte*>(W_CacheLumpName("TINTTAB", PU_STATIC));
}

//
// V_LoadXlaTable
//
// villsa [STRIFE] Load xla table from XLATAB lump.
//

void V_LoadXlaTable(void)
{
    xlatab = static_cast<byte*>(W_CacheLumpName("XLATAB", PU_STATIC));
}

//
// V_DrawBlock
// Draw a linear block of pixels into the view buffer.
//

void V_DrawBlock(int x, int y, int width, int height, byte *src)
{
    byte *dest;

#ifdef RANGECHECK
    if (x < 0
     || x + width >SCREENWIDTH
     || y < 0
     || y + height > SCREENHEIGHT)
    {
	I_Error ("Bad V_DrawBlock");
    }
#endif

    dest = dest_screen + y * SCREENWIDTH + x;

    while (height--)
    {
	memcpy (dest, src, width);
	src += width;
	dest += SCREENWIDTH;
    }
}

void V_DrawFilledBox(int x, int y, int w, int h, int c)
{
    uint8_t *buf, *buf1;
    int x1, y1;

    buf = I_VideoBuffer + SCREENWIDTH * y + x;

    for (y1 = 0; y1 < h; ++y1)
    {
        buf1 = buf;

        for (x1 = 0; x1 < w; ++x1)
        {
            *buf1++ = c;
        }

        buf += SCREENWIDTH;
    }
}

void V_DrawHorizLine(int x, int y, int w, int c)
{
    uint8_t *buf;
    int x1;

    buf = I_VideoBuffer + SCREENWIDTH * y + x;

    for (x1 = 0; x1 < w; ++x1)
    {
        *buf++ = c;
    }
}

void V_DrawVertLine(int x, int y, int h, int c)
{
    uint8_t *buf;
    int y1;

    buf = I_VideoBuffer + SCREENWIDTH * y + x;

    for (y1 = 0; y1 < h; ++y1)
    {
        *buf = c;
        buf += SCREENWIDTH;
    }
}

void V_DrawBox(int x, int y, int w, int h, int c)
{
    V_DrawHorizLine(x, y, w, c);
    V_DrawHorizLine(x, y+h-1, w, c);
    V_DrawVertLine(x, y, h, c);
    V_DrawVertLine(x+w-1, y, h, c);
}

//
// Draw a "raw" screen (lump containing raw data to blit directly
// to the screen)
//

void V_DrawRawScreen(byte *raw)
{
    memcpy(dest_screen, raw, SCREENWIDTH * SCREENHEIGHT);
}

//
// V_Init
//
void V_Init (void)
{
    // no-op!
    // There used to be separate screens that could be drawn to; these are
    // now handled in the upper layers.
}

// Set the buffer that the code draws to.

void V_UseBuffer(byte *buffer)
{
    dest_screen = buffer;
}

// Restore screen buffer to the i_video screen buffer.

void V_RestoreBuffer(void)
{
    dest_screen = I_VideoBuffer;
}

//
// SCREEN SHOTS
//

typedef struct
{
    char		manufacturer;
    char		version;
    char		encoding;
    char		bits_per_pixel;

    unsigned short	xmin;
    unsigned short	ymin;
    unsigned short	xmax;
    unsigned short	ymax;

    unsigned short	hres;
    unsigned short	vres;

    unsigned char	palette[48];

    char		reserved;
    char		color_planes;
    unsigned short	bytes_per_line;
    unsigned short	palette_type;

    char		filler[58];
    unsigned char	data;		// unbounded
} PACKEDATTR pcx_t;


//
// WritePCXfile
//

void WritePCXfile(const char *filename, const byte *data,
                  int width, int height,
                  byte *palette)
{
    int		i;
    int		length;
    pcx_t*	pcx;
    byte*	pack;

    pcx = static_cast<pcx_t*>(Z_Malloc (width*height*2+1000, PU_STATIC, NULL));

    pcx->manufacturer = 0x0a;		// PCX id
    pcx->version = 5;			// 256 color
    pcx->encoding = 1;			// uncompressed
    pcx->bits_per_pixel = 8;		// 256 color
    pcx->xmin = 0;
    pcx->ymin = 0;
    pcx->xmax = SHORT(width-1);
    pcx->ymax = SHORT(height-1);
    pcx->hres = SHORT(width);
    pcx->vres = SHORT(height);
    memset (pcx->palette,0,sizeof(pcx->palette));
    pcx->color_planes = 1;		// chunky image
    pcx->bytes_per_line = SHORT(width);
    pcx->palette_type = SHORT(2);	// not a grey scale
    memset (pcx->filler,0,sizeof(pcx->filler));

    // pack the image
    pack = &pcx->data;

    for (i=0 ; i<width*height ; i++)
    {
	if ( (*data & 0xc0) != 0xc0)
	    *pack++ = *data++;
	else
	{
	    *pack++ = 0xc1;
	    *pack++ = *data++;
	}
    }

    // write the palette
    *pack++ = 0x0c;	// palette ID byte
    for (i=0 ; i<768 ; i++)
	*pack++ = *palette++;

    // write output file
    length = pack - (byte *)pcx;
    M_WriteFile (const_cast<char*>(filename), pcx, length);

    Z_Free (pcx);
}

#ifdef HAVE_LIBPNG
//
// WritePNGfile
//

static void error_fn(png_structp p, png_const_charp s)
{
    DG_Log("libpng error: %s\n", s);
}

static void warning_fn(png_structp p, png_const_charp s)
{
    DG_Log("libpng warning: %s\n", s);
}

void WritePNGfile(char *filename, byte *data,
                  int width, int height,
                  byte *palette)
{
    png_structp ppng;
    png_infop pinfo;
    png_colorp pcolor;
    FILE *handle;
    int i;

    handle = fopen(filename, "wb");
    if (!handle)
    {
        return;
    }

    ppng = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
                                   error_fn, warning_fn);
    if (!ppng)
    {
        return;
    }

    pinfo = png_create_info_struct(ppng);
    if (!pinfo)
    {
        png_destroy_write_struct(&ppng, NULL);
        return;
    }

    png_init_io(ppng, handle);

    png_set_IHDR(ppng, pinfo, width, height,
                 8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    pcolor = malloc(sizeof(*pcolor) * 256);
    if (!pcolor)
    {
        png_destroy_write_struct(&ppng, &pinfo);
        return;
    }

    for (i = 0; i < 256; i++)
    {
        pcolor[i].red   = *(palette + 3 * i);
        pcolor[i].green = *(palette + 3 * i + 1);
        pcolor[i].blue  = *(palette + 3 * i + 2);
    }

    png_set_PLTE(ppng, pinfo, pcolor, 256);
    free(pcolor);

    png_write_info(ppng, pinfo);

    for (i = 0; i < SCREENHEIGHT; i++)
    {
        png_write_row(ppng, data + i*SCREENWIDTH);
    }

    png_write_end(ppng, pinfo);
    png_destroy_write_struct(&ppng, &pinfo);
    fclose(handle);
}
#endif

//
// V_ScreenShot
//

void V_ScreenShot(char *format)
{
    int i;
    char lbmname[16]; // haleyjd 20110213: BUG FIX - 12 is too small!
    const char *ext;

    // find a file name to save it to

#ifdef HAVE_LIBPNG
    extern int png_screenshots;
    if (png_screenshots)
    {
        ext = "png";
    }
    else
#endif
    {
        ext = "pcx";
    }

    for (i=0; i<=99; i++)
    {
        M_snprintf(lbmname, sizeof(lbmname), format, i, ext);

        if (!M_FileExists(lbmname))
        {
            break;      // file doesn't exist
        }
    }

    if (i == 100)
    {
        I_Error ("V_ScreenShot: Couldn't create a PCX");
    }

#ifdef HAVE_LIBPNG
    if (png_screenshots)
    {
    WritePNGfile(lbmname, I_VideoBuffer,
                 SCREENWIDTH, SCREENHEIGHT,
                 W_CacheLumpName (DEH_String("PLAYPAL"), PU_CACHE));
    }
    else
#endif
    {
    // save the pcx file
    WritePCXfile(lbmname, I_VideoBuffer,
                 SCREENWIDTH, SCREENHEIGHT,
                 static_cast<byte*>(W_CacheLumpName (DEH_String("PLAYPAL"), PU_CACHE)));
    }
}


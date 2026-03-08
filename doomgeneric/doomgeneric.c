#include <stdio.h>

#include "doomgeneric.h"

void D_DoomMain (void);


void doomgeneric_Create(const dg_screen_info_t* screen_info)
{
	DG_SetInitialScreenInfo(screen_info);
	DG_Init();

	D_DoomMain();
}

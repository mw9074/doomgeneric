#include <stdio.h>

#include "m_argv.h"

#include "doomgeneric.h"

void M_FindResponseFile(void);
void D_DoomMain (void);


void doomgeneric_Create(int argc, char** argv, const dg_screen_info_t* screen_info)
{
	// save arguments
	myargc = argc;
	myargv = argv;

	M_FindResponseFile();

	DG_SetInitialScreenInfo(screen_info);
	DG_Init();

	D_DoomMain();
}

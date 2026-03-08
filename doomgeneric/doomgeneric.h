#ifndef DOOM_GENERIC
#define DOOM_GENERIC

#include "doomgeneric_gfx.h"
#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void doomgeneric_Create(const dg_screen_info_t* screen_info);
void doomgeneric_Tick();


//Implement below functions for your platform
void DG_Init();
char* DG_GetFilesDir();
char* DG_GetDefaultConfigDir();
void DG_DrawFrame();
void DG_SleepMs(uint32_t ms);
uint32_t DG_GetTicksMs();
int DG_GetKey(int* pressed, unsigned char* key);
void DG_SetWindowTitle(const char * title);

#ifdef __cplusplus
}
#endif

#endif //DOOM_GENERIC

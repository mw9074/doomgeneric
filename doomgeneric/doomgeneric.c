#include "doomgeneric.h"
#include "doomgeneric_keys.h"
#include "doomkeys.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define KEYQUEUE_SIZE 16

static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

void D_DoomMain(void);

static unsigned short createControlKeyEvent(dg_key_state_t state, dg_control_key_t key);

static unsigned short createTextKeyEvent(dg_key_state_t state, char ch);

void doomgeneric_Create(const dg_screen_info_t* screen_info)
{
	memset(s_KeyQueue, 0, sizeof(s_KeyQueue));

	DG_SetInitialScreenInfo(screen_info);
	DG_Init();

	D_DoomMain();
}

int doomgeneric_GetQueuedKeyEvent(int* out_pressed, unsigned char* out_key)
{
	int result = 0;

	DG_KeyStateLock();

	if (s_KeyQueueReadIndex != s_KeyQueueWriteIndex)
	{
		// Key queue is not empty.
		unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
		s_KeyQueueReadIndex++;
		s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

		*out_pressed = keyData >> 8;
		*out_key = keyData & 0xFF;

		result = 1;
	}

	DG_KeyStateUnlock();

	return result;
}

void doomgeneric_QueueControlKeyEvent(dg_key_state_t state, dg_control_key_t key)
{
	unsigned short keyData = createControlKeyEvent(state, key);
	if (!keyData)
	{
		return;
	}

	DG_KeyStateLock();

	s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
	s_KeyQueueWriteIndex++;
	s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;

	DG_KeyStateUnlock();
}

void doomgeneric_QueueTextKeyEvent(dg_key_state_t state, char ch)
{
	unsigned short keyData = createTextKeyEvent(state, ch);
	if (!keyData)
	{
		return;
	}

	DG_KeyStateLock();

	s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
	s_KeyQueueWriteIndex++;
	s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;

	DG_KeyStateUnlock();
}

static unsigned short createControlKeyEvent(dg_key_state_t state, dg_control_key_t key)
{
	if (DG_KEY_STATE_PRESSED != state && DG_KEY_STATE_RELEASED != state)
	{
		// Invalid key state; ignore it.
		return 0;
	}

	unsigned short doomKey = 0;

	switch (key)
	{
	case DG_CONTROL_KEY_TAB:
		doomKey = KEY_TAB;
		break;
	case DG_CONTROL_KEY_ENTER:
		doomKey = KEY_ENTER;
		break;
	case DG_CONTROL_KEY_ESCAPE:
		doomKey = KEY_ESCAPE;
		break;
	case DG_CONTROL_KEY_SCREEN_SHRINK:
		doomKey = KEY_MINUS;
		break;
	case DG_CONTROL_KEY_SCREEN_EXPAND:
		doomKey = KEY_EQUALS;
		break;
	case DG_CONTROL_KEY_STRAFE_LEFT:
		doomKey = KEY_STRAFE_L;
		break;
	case DG_CONTROL_KEY_STRAFE_RIGHT:
		doomKey = KEY_STRAFE_R;
		break;
	case DG_CONTROL_KEY_USE:
		doomKey = KEY_USE;
		break;
	case DG_CONTROL_KEY_FIRE:
		doomKey = KEY_FIRE;
		break;
	case DG_CONTROL_KEY_LEFT:
		doomKey = KEY_LEFTARROW;
		break;
	case DG_CONTROL_KEY_UP:
		doomKey = KEY_UPARROW;
		break;
	case DG_CONTROL_KEY_RIGHT:
		doomKey = KEY_RIGHTARROW;
		break;
	case DG_CONTROL_KEY_DOWN:
		doomKey = KEY_DOWNARROW;
		break;
	case DG_CONTROL_KEY_RUN:
		doomKey = KEY_RSHIFT;
		break;
	case DG_CONTROL_KEY_BACKSPACE:
		doomKey = KEY_BACKSPACE;
		break;
	case DG_CONTROL_KEY_PAUSE:
		doomKey = KEY_PAUSE;
		break;
	case DG_CONTROL_KEY_DELETE:
		doomKey = KEY_DEL;
		break;
	}
	if (!doomKey)
	{
		return 0;
	}

	unsigned short keyData = ((unsigned short)state << 8) | doomKey;
	return keyData;
}

static unsigned short createTextKeyEvent(dg_key_state_t state, char ch)
{
	if (DG_KEY_STATE_PRESSED != state && DG_KEY_STATE_RELEASED != state)
	{
		// Invalid key state; ignore it.
		return 0;
	}

	if (ch < ' ' || ch > '~')
	{
		// Not a printable ASCII character; ignore it.
		return 0;
	}

	unsigned short keyData = ((unsigned short)state << 8) | (unsigned short)tolower(ch);
	return keyData;
}

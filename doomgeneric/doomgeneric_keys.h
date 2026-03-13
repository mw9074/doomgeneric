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
//  doomgeneric key types and functions.
//
#ifndef DOOMGENERIC_KEYS_H
#define DOOMGENERIC_KEYS_H

typedef enum dg_control_key {
	DG_CONTROL_KEY_NONE = 0,
	DG_CONTROL_KEY_TAB,
	DG_CONTROL_KEY_ENTER,
	DG_CONTROL_KEY_ESCAPE,
	DG_CONTROL_KEY_SCREEN_SHRINK,
	DG_CONTROL_KEY_SCREEN_EXPAND,
	DG_CONTROL_KEY_USE,
	DG_CONTROL_KEY_FIRE,
	DG_CONTROL_KEY_LEFT,
	DG_CONTROL_KEY_UP,
	DG_CONTROL_KEY_RIGHT,
	DG_CONTROL_KEY_DOWN,
	DG_CONTROL_KEY_STRAFE_LEFT,
	DG_CONTROL_KEY_STRAFE_RIGHT,
	DG_CONTROL_KEY_RUN,
	DG_CONTROL_KEY_BACKSPACE,
	DG_CONTROL_KEY_DELETE,
	DG_CONTROL_KEY_PAUSE,
} dg_control_key_t;

typedef enum dg_key_state {
	DG_KEY_STATE_RELEASED = 0,
	DG_KEY_STATE_PRESSED,
} dg_key_state_t;

#ifdef __cplusplus
extern "C" {
#endif

int doomgeneric_GetQueuedKeyEvent(int* out_pressed, unsigned char* out_key);

void doomgeneric_QueueControlKeyEvent(dg_key_state_t state, dg_control_key_t key);

void doomgeneric_QueueTextKeyEvent(dg_key_state_t state, char ch);

// Lock/Unlock the key state variables.
// Must be implemented by external code, must be reentrant.
void DG_KeyStateLock();
void DG_KeyStateUnlock();

#ifdef __cplusplus
}
#endif

#endif // DOOMGENERIC_KEYS_H

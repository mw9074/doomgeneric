#include "doomgeneric.h"
#include "doomgeneric_gfx.h"
#include "doomgeneric_keys.h"

#include <stdio.h>

#include <Windows.h>

#define DEFAULT_SCREEN_WIDTH 640
#define DEFAULT_SCREEN_HEIGHT 400

static BITMAPINFO s_Bmi = { sizeof(BITMAPINFOHEADER), DEFAULT_SCREEN_WIDTH, -DEFAULT_SCREEN_HEIGHT, 1, 32 };
static HWND s_Hwnd = 0;
static HDC s_Hdc = 0;

static char s_FilesDir[260] = ".";

static void addKeyEventToQueue(dg_key_state_t keyState, unsigned char keyCode)
{
	dg_control_key_t controlKey = DG_CONTROL_KEY_NONE;
	char ch = 0;

	switch (keyCode)
	{
	case VK_TAB:
		controlKey = DG_CONTROL_KEY_TAB;
		break;
	case VK_RETURN:
		controlKey = DG_CONTROL_KEY_ENTER;
		break;
	case VK_ESCAPE:
		controlKey = DG_CONTROL_KEY_ESCAPE;
		break;
	case VK_BACK:
		controlKey = DG_CONTROL_KEY_BACKSPACE;
		break;
	case VK_DELETE:
		controlKey = DG_CONTROL_KEY_DELETE;
		break;
	case VK_LEFT:
		controlKey = DG_CONTROL_KEY_LEFT;
		break;
	case VK_RIGHT:
		controlKey = DG_CONTROL_KEY_RIGHT;
		break;
	case VK_UP:
		controlKey = DG_CONTROL_KEY_UP;
		break;
	case VK_DOWN:
		controlKey = DG_CONTROL_KEY_DOWN;
		break;
	case VK_OEM_COMMA:
		controlKey = DG_CONTROL_KEY_STRAFE_LEFT;
		break;
	case VK_OEM_PERIOD:
		controlKey = DG_CONTROL_KEY_STRAFE_RIGHT;
		break;
	case VK_CONTROL:
		controlKey = DG_CONTROL_KEY_FIRE;
		break;
	case VK_SPACE:
		// If the spacebar is used, input both a control key
		// event for the "use" action and a text key event
		// for the space character, so that spaces can be
		// used in save game names.
		controlKey = DG_CONTROL_KEY_USE;
		ch = ' ';
		break;
	case VK_SHIFT:
		controlKey = DG_CONTROL_KEY_RUN;
		break;
	case VK_ADD:
	case VK_OEM_PLUS:
		controlKey = DG_CONTROL_KEY_SCREEN_EXPAND;
		break;
	case VK_SUBTRACT:
	case VK_OEM_MINUS:
		controlKey = DG_CONTROL_KEY_SCREEN_SHRINK;
		break;
	case VK_PAUSE:
		controlKey = DG_CONTROL_KEY_PAUSE;
		break;

	default:
		ch = (char)keyCode;
		break;
	}

	if (controlKey)
	{
		doomgeneric_QueueControlKeyEvent(keyState, controlKey);
	}
	if (ch)
	{
		// If the key is not a control key, try adding it as a text key.
		doomgeneric_QueueTextKeyEvent(keyState, ch);
	}
}

static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	RECT rect;
	int width, height;

	switch (msg)
	{
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		ExitProcess(0);
		break;
	case WM_SIZE:
		GetClientRect(hwnd, &rect);
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;

		// Only update the screen if it is at least
		// as big as the internal screen buffer.
		if (width >= 320 && height >= 200)
		{
			// Make width even to avoid an issue with the internal scaling code.
			width &= ~1;

			DG_SetScreenSize((uint32_t)width, (uint32_t)height);
		}
		return DefWindowProcA(hwnd, msg, wParam, lParam);
	case WM_KEYDOWN:
		addKeyEventToQueue(DG_KEY_STATE_PRESSED, (unsigned char)wParam);
		break;
	case WM_KEYUP:
		addKeyEventToQueue(DG_KEY_STATE_RELEASED, (unsigned char)wParam);
		break;
	default:
		return DefWindowProcA(hwnd, msg, wParam, lParam);
	}
	return 0;
}

void DG_GraphicsLock()
{
    // Nothing to do here since all usage of DG_ScreenBuffer is done on the same thread.
}

void DG_GraphicsUnlock()
{
    // Nothing to do here since all usage of DG_ScreenBuffer is done on the same thread.
}

void DG_KeyStateLock()
{
	// Nothing to do here since all usage of the key handling is done on the same thread.
}

void DG_KeyStateUnlock()
{
	// Nothing to do here since all usage of the key handling is done on the same thread.
}

void DG_Init()
{
	// window creation
	const char windowClassName[] = "DoomWindowClass";
	const char windowTitle[] = "Doom";
	WNDCLASSEXA wc;

	wc.cbSize = sizeof(WNDCLASSEXA);
	wc.style = 0;
	wc.lpfnWndProc = wndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = 0;
	wc.hIcon = 0;
	wc.hCursor = 0;
	wc.hbrBackground = 0;
	wc.lpszMenuName = 0;
	wc.lpszClassName = windowClassName;
	wc.hIconSm = 0;

	if (!RegisterClassExA(&wc))
	{
		DG_Log("Window Registration Failed!");

		exit(-1);
	}

	dg_screen_info_t si;
	DG_GetScreenInfo(&si);

	RECT rect;
	rect.left = rect.top = 0;
	rect.right = si.xres;
	rect.bottom = si.yres;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

	HWND hwnd = CreateWindowExA(0, windowClassName, windowTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, 0, 0, 0, 0);
	if (hwnd)
	{
		s_Hwnd = hwnd;

		s_Hdc = GetDC(hwnd);
		ShowWindow(hwnd, SW_SHOW);
	}
	else
	{
		DG_Log("Window Creation Failed!");

		exit(-1);
	}
}

char* DG_GetFilesDir()
{
    return s_FilesDir;
}

char* DG_GetDefaultConfigDir()
{
    char* dir = malloc(2);
    dir[0] = '.';
    dir[1] = '\0';
    return dir;
}

void DG_DrawFrame()
{
	MSG msg;
	memset(&msg, 0, sizeof(msg));

	while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	dg_screen_info_t si;
	DG_GetScreenInfo(&si);
	s_Bmi.bmiHeader.biWidth = si.xres;
	s_Bmi.bmiHeader.biHeight = -((LONG)si.yres); // negative height to indicate top-down bitmap

	StretchDIBits(s_Hdc, 0, 0, si.xres, si.yres, 0, 0, si.xres, si.yres, DG_ScreenBuffer, &s_Bmi, 0, SRCCOPY);

	SwapBuffers(s_Hdc);
}

void DG_SleepMs(uint32_t ms)
{
	Sleep(ms);
}

uint32_t DG_GetTicksMs()
{
	return GetTickCount();
}

void DG_SetWindowTitle(const char * title)
{
	if (s_Hwnd)
	{
		SetWindowTextA(s_Hwnd, title);
	}
}

int main(int argc, char** argv)
{
	dg_screen_info_t si = { 0 };
	si.color_format = DG_COLOR_FORMAT_RGBA8888;
	si.xres = DEFAULT_SCREEN_WIDTH;
	si.yres = DEFAULT_SCREEN_HEIGHT;

	doomgeneric_Create(&si);

	for (int i = 0; ; i++)
	{
		doomgeneric_Tick();
	}

	return 0;
}

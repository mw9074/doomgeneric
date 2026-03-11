#include "doomgeneric.h"
#include "doomgeneric_gfx.h"
#include "doomgeneric_keys.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <process.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define DEFAULT_SCREEN_WIDTH 640
#define DEFAULT_SCREEN_HEIGHT 400

static HWND s_Hwnd = 0;

static char s_FilesDir[260] = ".";

static bool s_DoomCreated = false;
static bool s_ExitDoomThread = false;
static HANDLE s_DoomThreadHandle = INVALID_HANDLE_VALUE;

static CRITICAL_SECTION s_DoomGfxCs;
static CRITICAL_SECTION s_DoomKeyStateCs;

static void InitCriticalSections();

static void DeleteCriticalSections();

static bool CreateWinObjects();

static void WinLoop();

static unsigned __stdcall doomThread(void* param);

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
		if (s_DoomCreated)
		{
			GetClientRect(hwnd, &rect);
			width = rect.right - rect.left;
			height = rect.bottom - rect.top;

			// Only update the screen if it is at least
			// as big as the internal screen buffer.
			if (width >= 320 && height >= 200)
			{
				// Make width even to avoid an issue with the internal scaling code.
				width &= ~1;

				doomgeneric_SetScreenSize((uint32_t)width, (uint32_t)height);
			}
		}
		return DefWindowProcA(hwnd, msg, wParam, lParam);
	case WM_PAINT:
		if (s_DoomCreated)
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);

			dg_screen_info_t si;
			doomgeneric_GetScreenInfo(&si);

			BITMAPINFO bmi = { sizeof(BITMAPINFOHEADER), si.xres, -((LONG)si.yres), 1, 32 };
			StretchDIBits(hdc, 0, 0, si.xres, si.yres, 0, 0, si.xres, si.yres, doomgeneric_ScreenBuffer, &bmi, 0, SRCCOPY);

			EndPaint(hwnd, &ps);
		}
		return 0;
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

static void InitCriticalSections()
{
	InitializeCriticalSection(&s_DoomGfxCs);
	InitializeCriticalSection(&s_DoomKeyStateCs);
}

static void DeleteCriticalSections()
{
	DeleteCriticalSection(&s_DoomGfxCs);
	DeleteCriticalSection(&s_DoomKeyStateCs);
}

static bool CreateWinObjects()
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
		return false;
	}

	RECT rect;
	rect.left = rect.top = 0;
	rect.right = DEFAULT_SCREEN_WIDTH;
	rect.bottom = DEFAULT_SCREEN_HEIGHT;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

	HWND hwnd = CreateWindowExA(0, windowClassName, windowTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, 0, 0, 0, 0);
	if (!hwnd)
	{
		DG_Log("Window Creation Failed!");
		return false;
	}

	s_Hwnd = hwnd;
	ShowWindow(hwnd, SW_SHOW);

	return true;
}

static void WinLoop()
{
	MSG msg;
	BOOL bRet;

	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (bRet == -1)
		{
			// handle the error and possibly exit
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void DG_GraphicsLock()
{
	EnterCriticalSection(&s_DoomGfxCs);
}

void DG_GraphicsUnlock()
{
	LeaveCriticalSection(&s_DoomGfxCs);
}

void DG_KeyStateLock()
{
	EnterCriticalSection(&s_DoomKeyStateCs);
}

void DG_KeyStateUnlock()
{
	LeaveCriticalSection(&s_DoomKeyStateCs);
}

void DG_Init()
{

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
	// Trigger the DOOM window to redraw on the main thread.
	InvalidateRect(s_Hwnd, NULL, FALSE);
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
	InitCriticalSections();

	if (!CreateWinObjects())
	{
		DeleteCriticalSections();
		return -1;
	}

	s_DoomThreadHandle = (HANDLE)_beginthreadex(NULL, 0, doomThread, NULL, 0, NULL);
	if (s_DoomThreadHandle)
	{
		WinLoop();

		// Signal the Doom thread to exit and wait for it to finish.
		s_ExitDoomThread = true;
		WaitForSingleObject(s_DoomThreadHandle, INFINITE);
	}

	DeleteCriticalSections();
	return 0;
}

static unsigned __stdcall doomThread(void* param)
{
	(void)param;

	dg_screen_info_t si = { 0 };
	si.color_format = DG_COLOR_FORMAT_RGBA8888;
	si.xres = DEFAULT_SCREEN_WIDTH;
	si.yres = DEFAULT_SCREEN_HEIGHT;

	doomgeneric_Create(&si);
	s_DoomCreated = true;

	while (!s_ExitDoomThread)
	{
		doomgeneric_Tick();
	}
	s_DoomCreated = false;

	return 0;
}

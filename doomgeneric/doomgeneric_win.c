#include "doomkeys.h"

#include "doomgeneric.h"
#include "doomgeneric_gfx.h"

#include <stdio.h>

#include <Windows.h>

#define DEFAULT_SCREEN_WIDTH 640
#define DEFAULT_SCREEN_HEIGHT 400

static BITMAPINFO s_Bmi = { sizeof(BITMAPINFOHEADER), DEFAULT_SCREEN_WIDTH, -DEFAULT_SCREEN_HEIGHT, 1, 32 };
static HWND s_Hwnd = 0;
static HDC s_Hdc = 0;


#define KEYQUEUE_SIZE 16

static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

static char s_FilesDir[260] = ".";

static unsigned char convertToDoomKey(unsigned char key)
{
	switch (key)
	{
	case VK_RETURN:
		key = KEY_ENTER;
		break;
	case VK_ESCAPE:
		key = KEY_ESCAPE;
		break;
	case VK_LEFT:
		key = KEY_LEFTARROW;
		break;
	case VK_RIGHT:
		key = KEY_RIGHTARROW;
		break;
	case VK_UP:
		key = KEY_UPARROW;
		break;
	case VK_DOWN:
		key = KEY_DOWNARROW;
		break;
	case VK_CONTROL:
		key = KEY_FIRE;
		break;
	case VK_SPACE:
		key = KEY_USE;
		break;
	case VK_SHIFT:
		key = KEY_RSHIFT;
		break;
	case VK_ADD:
	case VK_OEM_PLUS:
		// Increase size of rendered area
		key = KEY_EQUALS;
		break;
	case VK_SUBTRACT:
	case VK_OEM_MINUS:
		// Decrease size of rendered area
		key = KEY_MINUS;
		break;
	default:
		key = tolower(key);
		break;
	}

	return key;
}

static void addKeyToQueue(int pressed, unsigned char keyCode)
{
	unsigned char key = convertToDoomKey(keyCode);

	unsigned short keyData = (pressed << 8) | key;

	s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
	s_KeyQueueWriteIndex++;
	s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
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
		addKeyToQueue(1, wParam);
		break;
	case WM_KEYUP:
		addKeyToQueue(0, wParam);
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

	memset(s_KeyQueue, 0, KEYQUEUE_SIZE * sizeof(unsigned short));
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

int DG_GetKey(int* pressed, unsigned char* doomKey)
{
	if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex)
	{
		//key queue is empty

		return 0;
	}
	else
	{
		unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
		s_KeyQueueReadIndex++;
		s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

		*pressed = keyData >> 8;
		*doomKey = keyData & 0xFF;

		return 1;
	}
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

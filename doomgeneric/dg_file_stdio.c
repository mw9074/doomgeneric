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
//  doomgeneric C stdio file functions implementation of the dg_file_interface.
//

#include "dg_file_interface.h"

#ifdef DG_USE_STDIO_FILE_FUNCTIONS

#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

void DG_MakeDirectory(const char* path)
{
#ifdef _WIN32
	_mkdir(path);
#else
	mkdir(path, 0755);
#endif
}

dg_file_handle_t DG_FileOpen(const char* path, unsigned mode)
{
	char modeStr[4] = { 0 };

	if (mode & DG_FILE_MODE_READ)
	{
		modeStr[0] = 'r';
	}
	else if (mode & DG_FILE_MODE_WRITE)
	{
		modeStr[0] = 'w';
	}

	if (mode & DG_FILE_MODE_BINARY)
	{
		modeStr[1] = 'b';
	}

	if (0 == modeStr[0])
	{
		return NULL;
	}

	return fopen(path, modeStr);
}

void DG_FileClose(dg_file_handle_t handle)
{
	fclose(handle);
}

void DG_FileSeek(dg_file_handle_t handle, long offset, dg_file_seek_origin_t origin)
{
	int originFlag = 0;
	switch (origin)
	{
	case DG_FILE_SEEK_SET:
		originFlag = SEEK_SET;
		break;
	case DG_FILE_SEEK_END:
		originFlag = SEEK_END;
		break;
	default:
		return;
	}
	fseek(handle, offset, originFlag);
}

size_t DG_FileRead(dg_file_handle_t handle, void* buffer, size_t size)
{
	return fread(buffer, 1, size, handle);
}

size_t DG_FileWrite(dg_file_handle_t handle, const void* data, size_t size)
{
	return fwrite(data, 1, size, handle);
}

long DG_FileTell(dg_file_handle_t handle)
{
	return ftell(handle);
}

#endif // DG_USE_STDIO_FILE_FUNCTIONS

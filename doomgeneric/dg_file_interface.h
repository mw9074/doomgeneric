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
//  doomgeneric file interface, enabling non-standard file function implementations.
//
#ifndef DG_FILE_INTERFACE_H
#define DG_FILE_INTERFACE_H

#include <stddef.h>

// Comment out the following line if custom file functions are needed.
#define DG_USE_STDIO_FILE_FUNCTIONS

#ifdef DG_USE_STDIO_FILE_FUNCTIONS

#include <stdio.h>

typedef FILE* dg_file_handle_t;

#else

typedef struct dg_file {
	void* data;
} dg_file_t;

typedef dg_file_t* dg_file_handle_t;

#endif // DG_USE_STDIO_FILE_FUNCTIONS

typedef enum dg_file_mode {
	DG_FILE_MODE_READ = (1 << 0),
	DG_FILE_MODE_WRITE = (1 << 1),
	DG_FILE_MODE_BINARY = (1 << 2),
} dg_file_mode_t;

typedef enum dg_file_seek_origin {
	DG_FILE_SEEK_SET,
	DG_FILE_SEEK_END,
} dg_file_seek_origin_t;

#ifdef __cplusplus
extern "C" {
#endif

void DG_MakeDirectory(const char* path);

dg_file_handle_t DG_FileOpen(const char* path, unsigned mode);

void DG_FileClose(dg_file_handle_t handle);

void DG_FileSeek(dg_file_handle_t handle, long offset, dg_file_seek_origin_t origin);

size_t DG_FileRead(dg_file_handle_t handle, void* buffer, size_t size);

size_t DG_FileWrite(dg_file_handle_t handle, const void* data, size_t size);

long DG_FileTell(dg_file_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // DG_FILE_INTERFACE_H

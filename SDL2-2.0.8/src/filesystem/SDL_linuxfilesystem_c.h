/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2016 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "../SDL_internal.h"
#include "SDL_filesystem.h"

#ifndef _SDL_linuxfilesystem_c_h
#define _SDL_linuxfilesystem_c_h

SDL_bool linux_IsRootPath(const char* path);
SDL_bool linux_IsFromRootPath(const char* path);
SDL_bool linux_IsDirectory(const char* dir);
SDL_bool linux_IsFile(const char* file);

SDL_DIR* linux_OpenDir(char const* dir);
SDL_dirent2* linux_ReadDir(SDL_DIR* dir);
int linux_CloseDir(SDL_DIR* dir);
SDL_bool linux_GetStat(const char* name, SDL_dirent* stat);

SDL_bool linux_MakeDirectory(const char* dir);
SDL_bool linux_DeleteFiles(const char* name);
SDL_bool linux_CopyFiles(const char* src, const char* dst);
SDL_bool linux_RenameFile(const char* oldname, const char* newname);

#endif

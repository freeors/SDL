/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2018 Sam Lantinga <slouken@libsdl.org>

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
#include "../../SDL_internal.h"

#ifdef SDL_FILESYSTEM_ANDROID

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent filesystem routines                                */

#include <unistd.h>
#include <sys/stat.h>
#include <sys/statfs.h>

#include "SDL_error.h"
#include "SDL_filesystem.h"
#include "SDL_system.h"
#include "../SDL_linuxfilesystem_c.h"
#include "../../core/android/SDL_android.h"
#include "SDL_log.h"
#include "SDL_assert.h"

char *
SDL_GetBasePath(void)
{
    /* The current working directory is / on Android */
    SDL_Unsupported();
    return NULL;
}

char *
SDL_GetPrefPath(const char *org, const char *app)
{
    const char *path = SDL_AndroidGetInternalStoragePath();
    if (path) {
        size_t pathlen = SDL_strlen(path)+2;
        char *fullpath = (char *)SDL_malloc(pathlen);
        if (!fullpath) {
            SDL_OutOfMemory();
            return NULL;
        }
        SDL_snprintf(fullpath, pathlen, "%s/", path);
        return fullpath;
    }
    return NULL;
}

SDL_bool SDL_IsRootPath(const char* path)
{
	int size = SDL_strlen(path);
	if (size == 1) {
		return path[0] == '/'? SDL_TRUE: SDL_FALSE;
	}

	const char* res = "res/";
	if (size == 3) {
		return !SDL_strncmp(path, res, 3)? SDL_TRUE: SDL_FALSE;
	} else if (size == 4) {
		return !SDL_strncmp(path, res, 4)? SDL_TRUE: SDL_FALSE;
	}

	return SDL_FALSE;
}

SDL_bool SDL_IsFromRootPath(const char* path)
{
	const char* res = "res/";
	if (!path || (path[0] != '/' && (SDL_strlen(path) < 4 || SDL_strncmp(path, res, 4)))) {
		return SDL_FALSE;
	}
	return SDL_TRUE;
}

SDL_bool SDL_IsDirectory(const char* dir)
{
	if (*dir == '/') {
		return linux_IsDirectory(dir);
	}

	// Try from the asset system
	return Android_JNI_IsDirectory(dir);
}

SDL_bool SDL_IsFile(const char* file)
{
	if (*file == '/') {
		return linux_IsFile(file);
	}

	// Try from the asset system
	return Android_JNI_IsFile(file);
}

SDL_DIR* SDL_OpenDir(char const* dir)
{
	if (*dir == '/') {
		return linux_OpenDir(dir);
	}
	return Android_JNI_OpenDir(dir);
}

SDL_dirent2* SDL_ReadDir(SDL_DIR* dir)
{
	if (dir->directory[0] == '/') {
		return linux_ReadDir(dir);
	}
	return Android_JNI_ReadDir(dir);
}

int SDL_CloseDir(SDL_DIR* dir)
{
	if (dir->directory[0] == '/') {
		return linux_CloseDir(dir);
	}
	return Android_JNI_CloseDir(dir);
}

SDL_bool SDL_GetStat(const char* name, SDL_dirent* stat)
{
	if (*name == '/') {
		return linux_GetStat(name, stat);
	}
	return Android_JNI_GetStat(name, stat);
}

SDL_Photo* SDL_OpenPhoto(char const* dir)
{
	return Android_JNI_OpenPhoto(dir);
}

void* SDL_ReadPhoto(SDL_Photo* photos, const int index, const int width, const int height)
{
	return Android_JNI_ReadPhoto(photos, index, width, height);
}

int SDL_ClosePhoto(SDL_Photo* photos)
{
	return Android_JNI_ClosePhoto(photos);
}

SDL_bool SDL_MakeDirectory(const char* dir)
{
	return linux_MakeDirectory(dir);
}

SDL_bool SDL_DeleteFiles(const char* name)
{
	return linux_DeleteFiles(name);
}

SDL_bool SDL_CopyFiles(const char* src, const char* dst)
{
	return linux_CopyFiles(src, dst);
}

SDL_bool SDL_RenameFile(const char* oldname, const char* newname)
{
	return linux_RenameFile(oldname, newname);
}

int64_t SDL_DiskFreeSpace(const char* name)
{
	int64_t ret = 0;
	struct statfs buf;

	SDL_assert(name && name[0]);

	if (statfs(name, &buf) >= 0){
		ret = 1ll * buf.f_bsize * buf.f_bfree;
		SDL_Log("disk_free_space, name: %s, f_bsize: %i, f_bfree: %lld, ret: %lld\n", name? name: "<nil>", buf.f_bsize, buf.f_bfree, ret);
	}
	return ret;
}

void SDL_GetPlatformVariable(void** env, void** context, void** v3)
{
#ifndef _KOS
	// JavaVM* jvm, jobject context
	*env = (void*)Android_JNI_GetJavaVM();
	*context = (void*)Android_JNI_GetActivityObject();
#else
	SDL_Log("SDL_GetPlatformVariable, E");
	*env = NULL;
	*context = NULL;
#endif
}

char* SDL_CreateGUID(SDL_bool line)
{
	return Android_JNI_CreateGUID(line);
}

#endif /* SDL_FILESYSTEM_ANDROID */

/* vi: set ts=4 sw=4 expandtab: */

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

#ifdef SDL_FILESYSTEM_WINDOWS

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent filesystem routines                                */

#include "../../core/windows/SDL_windows.h"
#include <shlobj.h>

#include "SDL_assert.h"
#include "SDL_error.h"
#include "SDL_stdinc.h"
#include "SDL_filesystem.h"
#include <shellapi.h>

char *
SDL_GetBasePath(void)
{
    typedef DWORD (WINAPI *GetModuleFileNameExW_t)(HANDLE, HMODULE, LPWSTR, DWORD);
    GetModuleFileNameExW_t pGetModuleFileNameExW;
    DWORD buflen = 128;
    WCHAR *path = NULL;
    HANDLE psapi = LoadLibrary(L"psapi.dll");
    char *retval = NULL;
    DWORD len = 0;
    int i;

    if (!psapi) {
        WIN_SetError("Couldn't load psapi.dll");
        return NULL;
    }

    pGetModuleFileNameExW = (GetModuleFileNameExW_t)GetProcAddress(psapi, "GetModuleFileNameExW");
    if (!pGetModuleFileNameExW) {
        WIN_SetError("Couldn't find GetModuleFileNameExW");
        FreeLibrary(psapi);
        return NULL;
    }

    while (SDL_TRUE) {
        void *ptr = SDL_realloc(path, buflen * sizeof (WCHAR));
        if (!ptr) {
            SDL_free(path);
            FreeLibrary(psapi);
            SDL_OutOfMemory();
            return NULL;
        }

        path = (WCHAR *) ptr;

        len = pGetModuleFileNameExW(GetCurrentProcess(), NULL, path, buflen);
        if (len != buflen) {
            break;
        }

        /* buffer too small? Try again. */
        buflen *= 2;
    }

    FreeLibrary(psapi);

    if (len == 0) {
        SDL_free(path);
        WIN_SetError("Couldn't locate our .exe");
        return NULL;
    }

    for (i = len-1; i > 0; i--) {
        if (path[i] == '\\') {
            break;
        }
    }

    SDL_assert(i > 0); /* Should have been an absolute path. */
    path[i+1] = '\0';  /* chop off filename. */

    retval = WIN_StringToUTF8(path);
    SDL_free(path);

    return retval;
}

char *
SDL_GetPrefPath(const char *org, const char *app)
{
    /*
     * Vista and later has a new API for this, but SHGetFolderPath works there,
     *  and apparently just wraps the new API. This is the new way to do it:
     *
     *     SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE,
     *                          NULL, &wszPath);
     */

    WCHAR path[MAX_PATH];
    char *retval = NULL;
    WCHAR* worg = NULL;
    WCHAR* wapp = NULL;
    size_t new_wpath_len = 0;
    BOOL api_result = FALSE;

    if (!app) {
        SDL_InvalidParamError("app");
        return NULL;
    }
    if (!org) {
        org = "";
    }

    if (!SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, path))) {
        WIN_SetError("Couldn't locate our prefpath");
        return NULL;
    }

    worg = WIN_UTF8ToString(org);
    if (worg == NULL) {
        SDL_OutOfMemory();
        return NULL;
    }

    wapp = WIN_UTF8ToString(app);
    if (wapp == NULL) {
        SDL_free(worg);
        SDL_OutOfMemory();
        return NULL;
    }

    new_wpath_len = lstrlenW(worg) + lstrlenW(wapp) + lstrlenW(path) + 3;

    if ((new_wpath_len + 1) > MAX_PATH) {
        SDL_free(worg);
        SDL_free(wapp);
        WIN_SetError("Path too long.");
        return NULL;
    }

    if (*worg) {
        lstrcatW(path, L"\\");
        lstrcatW(path, worg);
    }
    SDL_free(worg);

    api_result = CreateDirectoryW(path, NULL);
    if (api_result == FALSE) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            SDL_free(wapp);
            WIN_SetError("Couldn't create a prefpath.");
            return NULL;
        }
    }

    lstrcatW(path, L"\\");
    lstrcatW(path, wapp);
    SDL_free(wapp);

    api_result = CreateDirectoryW(path, NULL);
    if (api_result == FALSE) {
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            WIN_SetError("Couldn't create a prefpath.");
            return NULL;
        }
    }

    lstrcatW(path, L"\\");

    retval = WIN_StringToUTF8(path);

    return retval;
}

SDL_bool SDL_IsRootPath(const char* path)
{
	int size = SDL_strlen(path);
	if (size == 2) {
		if (path[1] == ':') {
			return SDL_TRUE;
		}
	} else if (size == 3 && (path[2] == '/' || path[2] == '\\')) {
		if (path[1] == ':') {
			return SDL_TRUE;
		}
	}
	return SDL_FALSE;
}

SDL_bool SDL_IsFromRootPath(const char* path)
{
	if (!path || SDL_strlen(path) < 2 || path[1] != ':') {
		return SDL_FALSE;
	}
	return SDL_TRUE;
}

static WCHAR* adjust_wfile_if_necessary(WCHAR* wfile)
{
	int size = (int)SDL_wcslen(wfile);
	if (size >= MAX_PATH) {
		// once require add prefix, '/' must replace by '\\'.
		for (int at = 0; at < size; at++) {
			if (wfile[at] == L'/') {
				wfile[at] = L'\\';
			}
		}

		// delete all ..
		while (1) {
			int sep = -1;
			int exit = 1;
			for (int at = 0; at < size - 2; at++) {
				if (wfile[at] == L'\\') {
					if (wfile[at + 1] == L'.' && wfile[at + 2] == L'.') {
						// /..
						if (size - at - 3) {
							SDL_memcpy(wfile + sep, wfile + at + 3, (size - at - 3) * sizeof(WCHAR));
						}
						size -= at - sep + 3;
						wfile[size] = L'\0';
						exit = 0;
						break;
					}
					sep = at;
				}
			}
			if (exit) {
				break;
			}
		}

		if (size >= MAX_PATH) {
			WCHAR* tmp = SDL_malloc((size + 5) * sizeof(WCHAR));
			SDL_memcpy(tmp, L"\\\\?\\", 4 * sizeof(WCHAR));
			SDL_memcpy(tmp + 4, wfile, size * sizeof(WCHAR));
			tmp[4 + size] = L'\0';
			SDL_free(wfile);
			wfile = tmp;
		}
	}

	return wfile;
}

SDL_bool SDL_IsDirectory(const char* dir)
{
	WIN32_FILE_ATTRIBUTE_DATA fattrdata;
	BOOL fok;
	WCHAR* wdir = NULL;

	if (!dir || !dir[0]) {
		return SDL_FALSE;
	}
	wdir = WIN_UTF8ToString(dir);
	if (wdir == NULL) {
		SDL_OutOfMemory();
		return SDL_FALSE;
	}

	wdir = adjust_wfile_if_necessary(wdir);

	fok = GetFileAttributesEx(wdir, GetFileExInfoStandard, &fattrdata);
	SDL_free(wdir);
	if (!fok) {
		return SDL_FALSE;
	}
	return (fattrdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? SDL_TRUE : SDL_FALSE;
}

SDL_bool SDL_IsFile(const char* file)
{
	WIN32_FILE_ATTRIBUTE_DATA fattrdata;
	BOOL fok;
	WCHAR* wfile = NULL;

	if (!file || !file[0]) {
		return SDL_FALSE;
	}
	wfile = WIN_UTF8ToString(file);
	if (wfile == NULL) {
		SDL_OutOfMemory();
		return SDL_FALSE;
	}

	wfile = adjust_wfile_if_necessary(wfile);

	fok = GetFileAttributesEx(wfile, GetFileExInfoStandard, &fattrdata);
	SDL_free(wfile);
	if (!fok) {
		return SDL_FALSE;
	}
	return (fattrdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? SDL_FALSE : SDL_TRUE;
}

typedef struct
{
	WIN32_FIND_DATA find_data;	/* The Win32 FindFile data. */
	HANDLE hFind;		/* The Win32 FindFile handle. */
} SDL_WindowDIR;

static HANDLE dirent__findfile_directory(const WCHAR* name, LPWIN32_FIND_DATA data)
{
	WCHAR search_spec[_MAX_PATH + 1];

	// Simply add the *.*, ensuring the path separator is included.
	lstrcpy(search_spec, name);
	if (search_spec[lstrlen(search_spec) - 1] != '\\' && search_spec[lstrlen(search_spec) - 1] != '/') {
		lstrcat(search_spec, L"\\*.*");
	} else {
		lstrcat(search_spec, L"*.*");
	}

	return FindFirstFile(search_spec, data);
}

#define posix_mku64(l, h)		((uint64_t)(((uint32_t)(l)) | ((uint64_t)((uint32_t)(h))) << 32))    
#define FileTimeToUnixTime(ft)	\
	((posix_mku64((ft).dwLowDateTime, (ft).dwHighDateTime) - 116444736000000000I64) / 10000000)

SDL_DIR* SDL_OpenDir(char const* dir)
{
	WIN32_FILE_ATTRIBUTE_DATA fattrdata;
	BOOL fok;
	WCHAR* wdir = NULL;
	SDL_DIR* result = NULL;

	// Must be a valid name
	if (!dir || !dir[0]) {
		return NULL;
	}
	wdir = WIN_UTF8ToString(dir);
	if (wdir == NULL) {
		SDL_OutOfMemory();
		return NULL;
	}

	fok = GetFileAttributesEx(wdir, GetFileExInfoStandard, &fattrdata);
	if (!fok || !(fattrdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
		SDL_free(wdir);
		return NULL;
	}

	result = (SDL_DIR*)SDL_malloc(sizeof(SDL_DIR));
	SDL_memset(result, 0, sizeof(SDL_DIR));
	SDL_WindowDIR* data = (SDL_WindowDIR *)SDL_malloc(sizeof(*data));
	SDL_memset(data, 0, sizeof(*data));
	if (!data) {
		SDL_free(result);
		SDL_free(wdir);
		return NULL;
	}

	if (result) {
		data->hFind = dirent__findfile_directory(wdir, &data->find_data);
		if (data->hFind == INVALID_HANDLE_VALUE) {
			SDL_free(data);
			SDL_free(result);

			result = NULL;
		} else {
			/*
			// Save the directory, in case of rewind.
			char* utf8 = WIN_StringToUTF8(data->find_data.cFileName);
			SDL_strlcpy(result->dirent.d_name, utf8, sizeof(result->dirent.d_name));
			SDL_free(utf8);
			*/
			SDL_strlcpy(result->directory, dir, sizeof(result->directory));
			result->driverdata = data;
		}
	}
	SDL_free(wdir);

	return result;
}

SDL_dirent2* SDL_ReadDir(SDL_DIR* dir)
{
	SDL_WindowDIR* data = (SDL_WindowDIR*)dir->driverdata;
	// The last find exhausted the matches, so return NULL.
	if (data->hFind == INVALID_HANDLE_VALUE) {
		return NULL;

	} else {
		// Copy the result of the last successful match to dirent.
		char* utf8 = WIN_StringToUTF8(data->find_data.cFileName);
		SDL_strlcpy(dir->dirent.name, utf8, sizeof(dir->dirent.name));
		SDL_free(utf8);

		dir->dirent.mode = data->find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? SDL_S_IFDIR : SDL_S_IFREG;
		dir->dirent.size = posix_mku64(data->find_data.nFileSizeLow, data->find_data.nFileSizeHigh);
		dir->dirent.ctime = FileTimeToUnixTime(data->find_data.ftCreationTime);
		dir->dirent.mtime = FileTimeToUnixTime(data->find_data.ftLastWriteTime);
		dir->dirent.atime = FileTimeToUnixTime(data->find_data.ftLastAccessTime);

		// Attempt the next match.
		if (!FindNextFile(data->hFind, &data->find_data)) {
			// Exhausted all matches, so close and null the handle.
			FindClose(data->hFind);
			data->hFind = INVALID_HANDLE_VALUE;
		}

		return &dir->dirent;
	}
}

int SDL_CloseDir(SDL_DIR* dir)
{
	int ret = -1;
	if (dir) {
		SDL_WindowDIR* data = (SDL_WindowDIR*)dir->driverdata;
		// Close the search handle, if not already done.
		if (data->hFind != INVALID_HANDLE_VALUE) {
			FindClose(data->hFind);
		}
		SDL_free(dir->driverdata);
		SDL_free(dir);

		ret = 0;
	}

	return ret;
}

SDL_bool SDL_GetStat(const char* name, SDL_dirent* stat)
{
	WCHAR* wname = NULL;
	HANDLE fFile; // File Handle
	WIN32_FIND_DATA fileinfo; // File Information Structure

							  // Must be a valid name
	if (!name || !name[0]) {
		return SDL_FALSE;
	}
	wname = WIN_UTF8ToString(name);
	if (wname == NULL) {
		return SDL_FALSE;
	}

	fFile = FindFirstFile(wname, &fileinfo);
	SDL_free(wname);
	if (fFile == INVALID_HANDLE_VALUE) {
		return SDL_FALSE;
	}

	stat->mode = fileinfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? SDL_S_IFDIR : SDL_S_IFREG;
	stat->size = posix_mku64(fileinfo.nFileSizeLow, fileinfo.nFileSizeHigh);
	stat->ctime = FileTimeToUnixTime(fileinfo.ftCreationTime);
	stat->mtime = FileTimeToUnixTime(fileinfo.ftLastWriteTime);
	stat->atime = FileTimeToUnixTime(fileinfo.ftLastAccessTime);

	FindClose(fFile);
	return SDL_TRUE;
}

typedef struct
{
	char ret[512];
	char* data;	/* The Win32 FindFile data. */
	int* pos;		/* The Win32 FindFile handle. */
} SDL_WindowPhoto;

static SDL_bool is_image(SDL_dirent2* dirent)
{
	SDL_bool isdir = SDL_DIRENT_DIR(dirent->mode);
	if (isdir) {
		return SDL_FALSE;
	}

	const char* p = dirent->name;
	const char* p1 = p;
	while (p1) {
		p = p1;
		p1 = SDL_strchr(p1 + 1, '.');
	}
	if (*p != '.') {
		return SDL_FALSE;
	}
	if (SDL_strncasecmp(p + 1, "png", 3) == 0) {
		return SDL_TRUE;
	}
	if (SDL_strncasecmp(p + 1, "jpg", 3) == 0) {
		return SDL_TRUE;
	}
	if (SDL_strncasecmp(p + 1, "jpeg", 4) == 0) {
		return SDL_TRUE;
	}
	return SDL_FALSE;
}

static void* resize_data(const int size, void* data, const int vsize)
{
	void* tmp = SDL_malloc(size);
	if (data) {
		if (vsize) {
			SDL_memcpy(tmp, data, vsize);
		}
		SDL_free(data);
	}
	return tmp;
}

SDL_Photo* SDL_OpenPhoto(char const* dir)
{
	SDL_DIR* dir2 = SDL_OpenDir(dir);
	if (!dir2) {
		return NULL;
	}

	SDL_Photo* result = (SDL_Photo*)SDL_malloc(sizeof(SDL_Photo));
	SDL_memset(result, 0, sizeof(SDL_Photo));
	SDL_strlcpy(result->directory, dir, sizeof(result->directory));
	result->read_get_surface = SDL_FALSE;

	const int count_increase = 10;
	int max_count = count_increase * 2;

	const int data_increase = 50;
	int data_size = data_increase * 2;
	int data_vsize = 0;
	SDL_WindowPhoto* driverdata = (SDL_WindowPhoto*)SDL_malloc(sizeof(SDL_WindowPhoto));
	SDL_memset(driverdata, 0, sizeof(SDL_WindowPhoto));
	driverdata->data = (char*)SDL_malloc(data_size);
	driverdata->pos = (int*)SDL_malloc(max_count * sizeof(int));

	SDL_dirent2* dirent;
	do {
		dirent = SDL_ReadDir(dir2);
		if (dirent && is_image(dirent)) {
			if (result->count == max_count) {
				driverdata->pos = resize_data((max_count + count_increase) * sizeof(int), driverdata->pos, max_count * sizeof(int));
				max_count += count_increase;
			}
			driverdata->pos[result->count] = data_vsize;
			result->count ++;
			const int name_len = SDL_strlen(dirent->name);
			if (data_vsize + name_len + 1 > data_size) {
				driverdata->data = resize_data(data_size + data_increase, driverdata->data, data_size);
				data_size += data_increase;
			}
			SDL_memcpy(driverdata->data + data_vsize, dirent->name, name_len);
			driverdata->data[data_vsize + name_len] = '\0';
			data_vsize += name_len + 1;
		}
	} while (dirent);
	SDL_CloseDir(dir2);

	result->driverdata = driverdata;

	return result;
}

void* SDL_ReadPhoto(SDL_Photo* photos, const int index, const int width, const int height)
{
	SDL_WindowPhoto* driverdata = (SDL_WindowPhoto*)photos->driverdata;

	int directory_len = SDL_strlen(photos->directory);
	const char* name = driverdata->data + driverdata->pos[index];
	int name_len = SDL_strlen(name);

	SDL_memmove(driverdata->ret, photos->directory, directory_len);
	driverdata->ret[directory_len] = '/';
	SDL_memmove(driverdata->ret + directory_len + 1, name, name_len);
	driverdata->ret[directory_len + 1 + name_len] = '\0';

	return driverdata->ret;
}

int SDL_ClosePhoto(SDL_Photo* photos)
{
	int ret = -1;
	if (photos) {
		SDL_WindowPhoto* driverdata = (SDL_WindowPhoto*)photos->driverdata;
		if (driverdata->data) {
			SDL_free(driverdata->data);
		}
		if (driverdata->pos) {
			SDL_free(driverdata->pos);
		}
		SDL_free(driverdata);
		SDL_free(photos);

		ret = 0;
	}
	return ret;
}

SDL_bool SDL_MakeDirectory(const char* dir)
{
	WCHAR* wdir = NULL;
	SDL_bool ret = SDL_FALSE;
	HANDLE fFile; // File Handle
	WIN32_FIND_DATA fileinfo; // File Information Structure
	BOOL tt; // BOOL used to test if Create Directory was successful
	size_t x1 = 0; // Counter

	size_t size = SDL_strlen(dir);
	// last characer cannot / or \, FindFirstFile cannot process path that terminated by / or \.
	if (!dir || !dir[0] || dir[size - 1] == '/' || dir[size - 1] == '\\') {
		return SDL_FALSE;
	}
	wdir = WIN_UTF8ToString(dir);

	fFile = FindFirstFile(wdir, &fileinfo);

	// if the file exists and it is a directory
	if ((fFile != INVALID_HANDLE_VALUE) && (fileinfo.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)) {
		// Directory Exists close file and return
		FindClose(fFile);
		SDL_free(wdir);
		return FALSE;
	}
	// Close the file
	FindClose(fFile);

	SDL_bool require_create;
	// dir is utf8 string, recalculate length.
	size = SDL_wcslen(wdir);
	for (x1 = 0; x1 < size; x1++) { // Parse the supplied CString Directory String
		if (wdir[x1] != L'\\' && wdir[x1] != L'/') { // if the Charachter is not a \ or /    
			require_create = SDL_FALSE; // add the character to the Temp String
		}
		else {
			wdir[x1] = L'\0';
			require_create = SDL_TRUE;
		}

		if (x1 == size - 1) {
			require_create = SDL_TRUE; // If we reached the end of the String
		}
		if (require_create) {
			tt = CreateDirectory(wdir, NULL);
			// If the Directory exists it will return a false
			if (tt) {
				// If we were successful we set the attributes to normal
				SetFileAttributes(wdir, FILE_ATTRIBUTE_NORMAL);
			}
		}
		if (require_create && x1 != size - 1) {
			wdir[x1] = L'\\'; // recover original character.
		}
	}

	// Now lets see if the directory was successfully created
	fFile = FindFirstFile(wdir, &fileinfo);

	if (fileinfo.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
		// Directory Exists close file and return
		FindClose(fFile);
		ret = SDL_TRUE;
	} else {
		// For Some reason the Function Failed Return FALSE
		FindClose(fFile);
		ret = SDL_FALSE;
	}
	SDL_free(wdir);
	return ret;
}

// name maybe directory or file.
SDL_bool SDL_DeleteFiles(const char* name)
{
	WCHAR* wname;
	WIN32_FILE_ATTRIBUTE_DATA fattrdata;
	BOOL fok = TRUE;

	if (!name || !name[0]) {
		return SDL_FALSE;
	}
	wname = WIN_UTF8ToString(name);

	fok = GetFileAttributesEx(wname, GetFileExInfoStandard, &fattrdata);
	if (!fok) {
		SDL_free(wname);
		DWORD err = GetLastError();
		return err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND ? SDL_TRUE : SDL_FALSE;
	}
	if (fattrdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		// desire delete directory.
		SHFILEOPSTRUCT		fopstruct;
		size_t at, size = SDL_wcslen(wname);
		WCHAR* tmp = SDL_malloc((size + 2) * sizeof(WCHAR));
		SDL_memcpy(tmp, wname, (size + 1) * sizeof(WCHAR));
		tmp[size + 1] = L'\0'; // Note:  This string must be double-null terminated.
		for (at = 0; at < size; at++) {
			if (tmp[at] == L'/') {
				tmp[at] = L'\\';
			}
		}

		SDL_memset(&fopstruct, 0, sizeof(SHFILEOPSTRUCT));
		fopstruct.hwnd = NULL;
		fopstruct.wFunc = FO_DELETE;
		fopstruct.pFrom = tmp;
		fopstruct.fFlags = FOF_NOERRORUI | FOF_NOCONFIRMATION;
		fok = SHFileOperation(&fopstruct) ? FALSE : TRUE;

		if (!fok) {
			DWORD err = GetLastError();
			SDL_SetError("SDL_DeleteFiles, %i", err);
		}

		SDL_free(tmp);
	} else {
		// desire delete file. get ride of read-only.
		SetFileAttributes(wname, fattrdata.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);
		fok = DeleteFile(wname);
		if (!fok) {
			DWORD err = GetLastError();
			int ii = 0;
		}
	}

	SDL_free(wname);
	return fok ? SDL_TRUE : SDL_FALSE;
}

// both src and dst maybe directory or file.
// if copy directory, dst must be not exist.
SDL_bool SDL_CopyFiles(const char* src, const char* dst)
{
	WIN32_FILE_ATTRIBUTE_DATA fattrdata;
	BOOL fok = TRUE;

	if (!src || !src[0] || !dst || !dst[0]) {
		return SDL_FALSE;
	}

	WCHAR* wsrc = WIN_UTF8ToString(src);
	WCHAR* wdst = WIN_UTF8ToString(dst);

	fok = GetFileAttributesEx(wsrc, GetFileExInfoStandard, &fattrdata);
	if (!fok) {
		SDL_free(wsrc);
		SDL_free(wdst);
		return SDL_FALSE;
	}
	if (fattrdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		// desire copy directory.
		SHFILEOPSTRUCT		fopstruct;

		SDL_memset(&fopstruct, 0, sizeof(SHFILEOPSTRUCT));
		fopstruct.hwnd = NULL;
		fopstruct.wFunc = FO_COPY;

		size_t at, size = SDL_wcslen(wsrc);
		WCHAR* tsrc = SDL_malloc((size + 2) * sizeof(WCHAR));
		SDL_memcpy(tsrc, wsrc, (size + 1) * sizeof(WCHAR));
		tsrc[size + 1] = L'\0'; // Note:  This string must be double-null terminated.
		for (at = 0; at < size; at++) {
			if (tsrc[at] == L'/') {
				tsrc[at] = L'\\';
			}
		}

		size = SDL_wcslen(wdst);
		WCHAR* tdst = SDL_malloc((size + 2) * sizeof(WCHAR));
		SDL_memcpy(tdst, wdst, (size + 1) * sizeof(WCHAR));
		tdst[size + 1] = L'\0'; // Note:  This string must be double-null terminated.
		for (at = 0; at < size; at++) {
			if (tdst[at] == L'/') {
				tdst[at] = L'\\';
			}
		}

		fopstruct.pFrom = tsrc;
		fopstruct.pTo = tdst;

		fopstruct.fFlags = FOF_NOERRORUI | FOF_NOCONFIRMATION;
		fok = SHFileOperation(&fopstruct) ? FALSE : TRUE;

		SDL_free(tsrc);
		SDL_free(tdst);

	} else {
		// desire copy file.
		fok = CopyFile(wsrc, wdst, FALSE);
	}

	SDL_free(wsrc);
	SDL_free(wdst);
	return fok ? SDL_TRUE : SDL_FALSE;
}

// oldname is full name.
// newname is terminate name. full new name is base_path(oldname) + newname
SDL_bool SDL_RenameFile(const char* oldname, const char* newname)
{
	WIN32_FILE_ATTRIBUTE_DATA fattrdata;
	BOOL fok = TRUE;

	if (!oldname || !oldname[0] || !newname || !newname[0]) {
		return SDL_FALSE;
	}

	// oldname is full name.
	int last_split_pos = -1, at, size = SDL_strlen(oldname);
	if (oldname[size - 1] == '\\' || oldname[size - 1] == '/') {
		return SDL_FALSE;
	}
	for (at = size - 1; at >= 0; at--) {
		if (oldname[at] == '\\' || oldname[at] == '/') {
			last_split_pos = at;
			break;
		}
	}
	if (last_split_pos == -1) {
		return SDL_FALSE;
	}
	if (!SDL_strcasecmp(oldname + last_split_pos + 1, newname)) {
		return SDL_TRUE;
	}

	// newname is termimate name.
	size = SDL_strlen(newname);
	for (at = 0; at < size; at++) {
		if (newname[at] == '\\' || newname[at] == '/' || newname[at] == ':') {
			return SDL_FALSE;
		}
	}

	WCHAR* wold = WIN_UTF8ToString(oldname);

	fok = GetFileAttributesEx(wold, GetFileExInfoStandard, &fattrdata);
	if (!fok) {
		SDL_free(wold);
		return SDL_FALSE;
	}

	char* newname2 = SDL_malloc(last_split_pos + 1 + size + 1);
	SDL_memcpy(newname2, oldname, last_split_pos + 1);
	SDL_memcpy(newname2 + last_split_pos + 1, newname, size);
	newname2[last_split_pos + 1 + size] = '\0';
	WCHAR* wnew = WIN_UTF8ToString(newname2);

	if (fattrdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		// desire rename directory.
		if (SDL_IsFile(newname2)) {
			SDL_free(newname2);
			SDL_free(wold);
			SDL_free(wnew);
			return SDL_FALSE;

		} else if (SDL_IsDirectory(newname2)) {
			if (!SDL_DeleteFiles(newname2)) {
				SDL_free(newname2);
				SDL_free(wold);
				SDL_free(wnew);
				return SDL_FALSE;
			}
		}

	}
	else {
		// desire rename file.
		if (SDL_IsDirectory(newname2)) {
			SDL_free(newname2);
			SDL_free(wold);
			SDL_free(wnew);
			return SDL_FALSE;
		} else if (SDL_IsFile(newname2)) {
			if (!SDL_DeleteFiles(newname2)) {
				SDL_free(newname2);
				SDL_free(wold);
				SDL_free(wnew);
				return SDL_FALSE;
			}
		}
	}
	{
		SHFILEOPSTRUCT		fopstruct;

		SDL_memset(&fopstruct, 0, sizeof(SHFILEOPSTRUCT));
		fopstruct.hwnd = NULL;
		fopstruct.wFunc = FO_RENAME;

		size_t at, size = SDL_wcslen(wold);
		WCHAR* told = SDL_malloc((size + 2) * sizeof(WCHAR));
		SDL_memcpy(told, wold, (size + 1) * sizeof(WCHAR));
		told[size + 1] = L'\0'; // Note:  This string must be double-null terminated.
		for (at = 0; at < size; at++) {
			if (told[at] == L'/') {
				told[at] = L'\\';
			}
		}

		size = SDL_wcslen(wnew);
		WCHAR* tnew = SDL_malloc((size + 2) * sizeof(WCHAR));
		SDL_memcpy(tnew, wnew, (size + 1) * sizeof(WCHAR));
		tnew[size + 1] = L'\0'; // Note:  This string must be double-null terminated.
		for (at = 0; at < size; at++) {
			if (tnew[at] == L'/') {
				tnew[at] = L'\\';
			}
		}

		fopstruct.pFrom = told;
		fopstruct.pTo = tnew;

		// fopstruct.fFlags = FOF_NOERRORUI | FOF_NOCONFIRMATION;
		fok = SHFileOperation(&fopstruct) ? FALSE : TRUE;

		SDL_free(told);
		SDL_free(tnew);

	}

	SDL_free(newname2);
	SDL_free(wold);
	SDL_free(wnew);
	return fok ? SDL_TRUE : SDL_FALSE;
}

int64_t SDL_DiskFreeSpace(const char* name)
{
	int64_t ret = 0;
	SDL_assert(name && name[0]);

	int size = SDL_strlen(name);
	if (size >= 2 && name[1] == ':') {
		char name2[4] = {name[0], name[1], '\\', '\0'};
		DWORD sectors_per_cluster, bytes_per_sector, free_clusters, clusters;

		GetDiskFreeSpaceA(name2, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &clusters);
		ret = 1i64 * sectors_per_cluster * bytes_per_sector * free_clusters;
	}

	return ret;
}

void SDL_GetPlatformVariable(void** v1, void** v2, void** v3)
{
}

char* SDL_CreateGUID(SDL_bool line)
{
	GUID guid;
	CoCreateGuid(&guid);

	int len = 40;
	char* result = SDL_malloc(len);
	SDL_memset(result, 0, len);
	if (line) {
		SDL_snprintf(result, len,
			"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
			guid.Data1, guid.Data2, guid.Data3,
			guid.Data4[0], guid.Data4[1], guid.Data4[2],
			guid.Data4[3], guid.Data4[4], guid.Data4[5],
			guid.Data4[6], guid.Data4[7]);
	} else {
		SDL_snprintf(result, len,
			"%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x",
			guid.Data1, guid.Data2, guid.Data3,
			guid.Data4[0], guid.Data4[1], guid.Data4[2],
			guid.Data4[3], guid.Data4[4], guid.Data4[5],
			guid.Data4[6], guid.Data4[7]);
	}
    return result;
}

#endif /* SDL_FILESYSTEM_WINDOWS */

/* vi: set ts=4 sw=4 expandtab: */

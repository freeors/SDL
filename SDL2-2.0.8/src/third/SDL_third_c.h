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
#include "SDL_third.h"

#ifndef _SDL_third_c_h
#define _SDL_third_c_h

typedef struct 
{
	int (*IsAppInstalled)(int app);
	int (*Login)(int app, SDL_LoginCallback callback, const char* secretkey1, const char* secretkey2, void* param);
	int (*Send)(int app, int scene, int type, const uint8_t* data, const int len, const char* name, const char* title, const char* desc, const uint8_t* thumb, const int thumb_len, SDL_SendCallback callback, void* param);
} SDL_MiniThird;

typedef struct
{
	int (*available) (void);
	SDL_MiniThird* (*create) (void);
} ThirdBootStrap;

#if __WIN32__
extern ThirdBootStrap WINDOWS_third;
#endif
#if __IPHONEOS__
extern ThirdBootStrap UIKIT_third;
#endif
#if __ANDROID__
extern ThirdBootStrap Android_third;
#endif

#endif

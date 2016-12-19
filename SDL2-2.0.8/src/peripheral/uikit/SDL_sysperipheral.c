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

#include "SDL_peripheral.h"

#ifndef posix_mku32
	#define posix_mku32(l, h)		((uint32_t)(((uint16_t)(l)) | ((uint32_t)((uint16_t)(h))) << 16))
#endif

#ifndef posix_mku16
	#define posix_mku16(l, h)		((uint16_t)(((uint8_t)(l)) | ((uint16_t)((uint8_t)(h))) << 8))
#endif

SDL_bool SDL_ReadCard(SDL_IDCardInfo* info)
{
	SDL_memset(info, 0, sizeof(SDL_IDCardInfo));
	info->type = SDL_CardIDCard;
	SDL_strlcpy(info->name, "ancientcc", sizeof(info->name));
	info->gender = SDL_GenderMale;
	info->folk = 1;
	info->birthday = posix_mku32(posix_mku16(4, 12), 1979);
	SDL_strlcpy(info->number, "33992219791204401X", sizeof(info->number));

	return SDL_TRUE;
}

/* vi: set ts=4 sw=4 expandtab: */

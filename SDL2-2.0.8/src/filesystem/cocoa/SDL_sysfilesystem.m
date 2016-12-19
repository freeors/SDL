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

#ifdef SDL_FILESYSTEM_COCOA

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent filesystem routines                                */

#include <Foundation/Foundation.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "SDL_error.h"
#include "SDL_stdinc.h"
#include "SDL_filesystem.h"
#include "SDL_linuxfilesystem_c.h"
#include "SDL_surface.h"

#import <Photos/Photos.h>

char *
SDL_GetBasePath(void)
{ @autoreleasepool
{
    NSBundle *bundle = [NSBundle mainBundle];
    const char* baseType = [[[bundle infoDictionary] objectForKey:@"SDL_FILESYSTEM_BASE_DIR_TYPE"] UTF8String];
    const char *base = NULL;
    char *retval = NULL;

    if (baseType == NULL) {
        baseType = "resource";
    }
    if (SDL_strcasecmp(baseType, "bundle")==0) {
        base = [[bundle bundlePath] fileSystemRepresentation];
    } else if (SDL_strcasecmp(baseType, "parent")==0) {
        base = [[[bundle bundlePath] stringByDeletingLastPathComponent] fileSystemRepresentation];
    } else {
        /* this returns the exedir for non-bundled  and the resourceDir for bundled apps */
        base = [[bundle resourcePath] fileSystemRepresentation];
    }

    if (base) {
        const size_t len = SDL_strlen(base) + 2;
        retval = (char *) SDL_malloc(len);
        if (retval == NULL) {
            SDL_OutOfMemory();
        } else {
            SDL_snprintf(retval, len, "%s/", base);
        }
    }

    return retval;
}}

char *
SDL_GetPrefPath(const char *org, const char *app)
{ @autoreleasepool
{
    if (!app) {
        SDL_InvalidParamError("app");
        return NULL;
    }
    if (!org) {
        org = "";
    }

    char *retval = NULL;
    NSArray *array = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);

    if ([array count] > 0) {  /* we only want the first item in the list. */
        NSString *str = [array objectAtIndex:0];
        const char *base = [str fileSystemRepresentation];
        if (base) {
            const size_t len = SDL_strlen(base) + SDL_strlen(org) + SDL_strlen(app) + 4;
            retval = (char *) SDL_malloc(len);
            if (retval == NULL) {
                SDL_OutOfMemory();
            } else {
                char *ptr;
                if (*org) {
                    SDL_snprintf(retval, len, "%s/%s/%s/", base, org, app);
                } else {
                    SDL_snprintf(retval, len, "%s/%s/", base, app);
                }
                for (ptr = retval+1; *ptr; ptr++) {
                    if (*ptr == '/') {
                        *ptr = '\0';
                        mkdir(retval, 0700);
                        *ptr = '/';
                    }
                }
                mkdir(retval, 0700);
            }
        }
    }

    return retval;
}}

SDL_bool SDL_IsRootPath(const char* path)
{
	return linux_IsRootPath(path);
}

SDL_bool SDL_IsFromRootPath(const char* path)
{
	return linux_IsFromRootPath(path);
}

SDL_bool SDL_IsDirectory(const char* dir)
{
	return linux_IsDirectory(dir);
}

SDL_bool SDL_IsFile(const char* file)
{
	return linux_IsFile(file);
}

SDL_DIR* SDL_OpenDir(char const* dir)
{
	return linux_OpenDir(dir);
}

SDL_dirent2* SDL_ReadDir(SDL_DIR* dir)
{
	return linux_ReadDir(dir);
}

int SDL_CloseDir(SDL_DIR* dir)
{
	return linux_CloseDir(dir);
}

SDL_bool SDL_GetStat(const char* name, SDL_dirent* stat)
{
	return linux_GetStat(name, stat);
}

// Return a bitmap context using alpha/red/green/blue byte values
CGContextRef CreateRGBABitmapContext (CGImageRef inImage)
{
	CGContextRef context = NULL;
	CGColorSpaceRef colorSpace;
	void *bitmapData;
	int bitmapByteCount;
	int bitmapBytesPerRow;

	size_t pixelsWide = CGImageGetWidth(inImage);
	size_t pixelsHigh = CGImageGetHeight(inImage);
	bitmapBytesPerRow = (pixelsWide * 4);
    bitmapByteCount = (bitmapBytesPerRow * pixelsHigh);
	colorSpace = CGColorSpaceCreateDeviceRGB();
	if (colorSpace == NULL){
		fprintf(stderr, "Error allocating color space");
		return NULL;
	}

	// allocate the bitmap & create context
	bitmapData = SDL_malloc(bitmapByteCount);
	if (bitmapData == NULL) {
		printf(stderr, "Memory not allocated!");
		CGColorSpaceRelease( colorSpace );
		return NULL;
	} 
	context = CGBitmapContextCreate(bitmapData, pixelsWide,pixelsHigh, 8, bitmapBytesPerRow, colorSpace, kCGImageAlphaPremultipliedLast);
	if (context == NULL) {
		SDL_free(bitmapData);
		fprintf (stderr, "Context not created!");
	}  
	CGColorSpaceRelease(colorSpace);
	return context;
}

unsigned char* RequestImagePixelData(UIImage *inImage)
{
	CGImageRef img = [inImage CGImage];
	CGSize size = [inImage size];
	CGContextRef cgctx = CreateRGBABitmapContext(img);
	if (cgctx == NULL) {
		return NULL;
	}
	CGRect rect = {{0,0}, {size.width, size.height}};
	CGContextDrawImage(cgctx, rect, img);
	unsigned char* data = CGBitmapContextGetData(cgctx);
	CGContextRelease(cgctx);
	return data;
}

typedef struct
{
	void* fetchResult;
} SDL_UiKitPhoto;

SDL_Photo* SDL_OpenPhoto(char const* dir)
{
    SDL_bool authorizated = SDL_FALSE;
    PHAuthorizationStatus status = [PHPhotoLibrary authorizationStatus];
    if (status == PHAuthorizationStatusRestricted ||
        status == PHAuthorizationStatusDenied) {
        // no photo authorization
        // popup a dialog, left user to enable photo authorization
    }  else {
        // has photo autorization
        authorizated = SDL_TRUE;
    }
    
	SDL_Photo* result = (SDL_Photo*)SDL_malloc(sizeof(SDL_Photo));
	SDL_memset(result, 0, sizeof(SDL_Photo));
	SDL_strlcpy(result->directory, dir, sizeof(result->directory));
	result->read_get_surface = SDL_TRUE;

	PHFetchOptions *option = [[PHFetchOptions alloc] init];
    option.predicate = [NSPredicate predicateWithFormat:@"mediaType == %ld", PHAssetMediaTypeImage];
    option.sortDescriptors = @[[NSSortDescriptor sortDescriptorWithKey:@"creationDate" ascending:NO]];
        
    PHFetchResult *smartAlbums = [PHAssetCollection fetchAssetCollectionsWithType:PHAssetCollectionTypeSmartAlbum subtype:PHAssetCollectionSubtypeSmartAlbumUserLibrary options:nil];
    
    // get all photos in camera
	for (PHAssetCollection *collection in smartAlbums) {
		PHFetchResult *fetchResult = [PHAsset fetchAssetsInAssetCollection:collection options:option];
        NSLog(@"sub album title is %@, count is %ld", collection.localizedTitle, (unsigned long)fetchResult.count);

        if ([collection.localizedTitle isEqualToString:@"Camera Roll"]) {
			result->driverdata = (void *)CFBridgingRetain(fetchResult);

            // below statement will print count = 2.
			// printf("2, retain count = %ld\n", CFGetRetainCount((__bridge CFTypeRef)(fetchResult)));

			result->count = fetchResult.count;
			break;
        }
    }
    
    return result;
}

void* SDL_ReadPhoto(SDL_Photo* photos, const int index, const int width, const int height)
{
	PHFetchResult* fetchResult = (__bridge PHFetchResult *)(photos->driverdata);
	PHAsset* asset = fetchResult[index];

    // NSString *filename = [asset valueForKey:@"filename"];
                
    // this gets called for every asset from its localIdentifier you saved
    PHImageRequestOptions * imageRequestOptions = [[PHImageRequestOptions alloc] init];
    imageRequestOptions.synchronous = YES;
	imageRequestOptions.resizeMode = PHImageRequestOptionsResizeModeExact;
    imageRequestOptions.deliveryMode = PHImageRequestOptionsResizeModeFast;
                                
    CGSize size = width != -1? CGSizeMake(width, height): PHImageManagerMaximumSize;

	__block SDL_Surface* surface = NULL;
    [[PHImageManager defaultManager]requestImageForAsset:asset targetSize:size contentMode:PHImageContentModeAspectFill options:imageRequestOptions resultHandler:^(UIImage * _Nullable result, NSDictionary * _Nullable info) {

		CGSize size = [result size];
        // NSLog(@"#%i, filename:%@, size:(%f x %f)", index, filename, size.width, size.height);

		unsigned char* data = RequestImagePixelData(result);
		surface = SDL_CreateRGBSurfaceFrom(data, size.width, size.height, 4 * 8, 4 * size.width,
			0xFF, 0xFF00, 0xFF0000, 0xFF000000); // SDL_PIXELFORMAT_ABGR8888
    }];
	return surface;
}

int SDL_ClosePhoto(SDL_Photo* photos)
{
	int ret = -1;
	if (photos && photos->driverdata) {
        // below statement will increate one reference count.
		// PHFetchResult* fetchResult = (__bridge PHFetchResult *)(photos->driverdata);
        
		CFRelease(photos->driverdata);

		SDL_free(photos);
        
		ret = 0;
	}
	return ret;
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

void SDL_GetPlatformVariable(void** v1, void** v2, void** v3)
{
}

#endif /* SDL_FILESYSTEM_COCOA */

/* vi: set ts=4 sw=4 expandtab: */

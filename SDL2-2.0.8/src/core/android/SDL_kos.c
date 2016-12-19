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
#include "SDL_stdinc.h"
#include "SDL_assert.h"
#include "SDL_hints.h"
#include "SDL_log.h"
#include "SDL_main.h"

#ifdef __ANDROID__

#include "SDL_system.h"
#include "SDL_android.h"

#include "keyinfotable.h"

#include "../../events/SDL_events_c.h"
#include "../../video/android/SDL_androidkeyboard.h"
#include "../../video/android/SDL_androidmouse.h"
#include "../../video/android/SDL_androidtouch.h"
#include "../../video/android/SDL_androidvideo.h"
#include "../../video/android/SDL_androidwindow.h"
#include "../../joystick/android/SDL_sysjoystick_c.h"
#include "../../haptic/android/SDL_syshaptic_c.h"

#include <android/log.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <dlfcn.h>
#include <android/bitmap.h>

#include <kosapi/gui2.h>
#include <kosapi/sys2.h>

/* Uncomment this to log messages entering and exiting methods in this file */
/* #define DEBUG_JNI */

/*******************************************************************************
 This file links the Java side of Android with libsdl
*******************************************************************************/
#include <jni.h>


/*******************************************************************************
                               Globals
*******************************************************************************/

/* Accelerometer data storage */
static float fLastAccelerometer[3];
static SDL_bool bHasNewData;

/*******************************************************************************
                 Functions called by JNI
*******************************************************************************/

/* Library init */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    SDL_Log("JNI_OnLoad------");
    return JNI_VERSION_1_4;
}

void checkJNIReady()
{
    SDL_Log("checkJNIReady------");
    SDL_SetMainReady();    
}

SDL_bool orientation_changed = SDL_FALSE;

/*******************************************************************************
             Functions called by SDL into Java
*******************************************************************************/

ANativeWindow* Android_JNI_GetNativeWindow(void)
{
	ANativeWindow* anw;

	SDL_Log("Android_JNI_GetNativeWindow, E, _KOS");
	sys2_set_did(Android_OnTouch, Android_OnMouse);
	anw = gui2_get_surface(NULL);
	SDL_Log("Android_JNI_GetNativeWindow, X, _KOS, anw: %p", anw);

    return anw;
}

void Android_JNI_SetActivityTitle(const char *title)
{
    SDL_Log("------Android_JNI_SetActivityTitle(title: %s)------", title != NULL? title: "<nil>");
}

void Android_JNI_SetWindowStyle(SDL_bool fullscreen)
{
    SDL_Log("------Android_JNI_SetWindowStyle(fullscreen: %s)------", fullscreen? "true": "false");
}

void Android_JNI_SetOrientation(int w, int h, int resizable, const char *hint)
{
	SDL_Log("------Android_JNI_SetOrientation(%ix%i, resizable: %i)------", w, h, resizable);
}

SDL_bool Android_JNI_GetAccelerometerValues(float values[3])
{
	SDL_Log("------Android_JNI_GetAccelerometerValues------");
    int i;
    SDL_bool retval = SDL_FALSE;

    if (bHasNewData) {
        for (i = 0; i < 3; ++i) {
            values[i] = fLastAccelerometer[i];
        }
        bHasNewData = SDL_FALSE;
        retval = SDL_TRUE;
    }

    return retval;
}

JNIEnv* Android_JNI_GetEnv(void)
{
    SDL_Log("------Android_JNI_GetEnv------");
    return NULL;
}

int Android_JNI_SetupThread(void)
{
    SDL_Log("------Android_JNI_SetupThread------");
    return 1;
}

/*
 * Audio support
 */

int Android_JNI_GetDisplayDPI(float *ddpi, float *xdpi, float *ydpi)
{
    SDL_Log("Android_JNI_GetDisplayDPI------");
    return 0;
}

void * Android_JNI_GetAudioBuffer(void)
{
    return NULL;
}

int Android_JNI_SetClipboardText(const char* text)
{
    SDL_Log("Android_JNI_SetClipboardText------");
    return 0;
}

char* Android_JNI_GetClipboardText(void)
{
    SDL_Log("Android_JNI_GetClipboardText------");
	return NULL;
}

SDL_bool Android_JNI_HasClipboardText(void)
{
    SDL_Log("Android_JNI_HasClipboardText------");
    return SDL_TRUE;
}

/* returns 0 on success or -1 on error (others undefined then)
 * returns truthy or falsy value in plugged, charged and battery
 * returns the value in seconds and percent or -1 if not available
 */
int Android_JNI_GetPowerInfo(int* plugged, int* charged, int* battery, int* seconds, int* percent)
{
    SDL_Log("Android_JNI_GetPowerInfo------");
    return 0;
}

/* returns number of found touch devices as return value and ids in parameter ids */
int Android_JNI_GetTouchDeviceIds(int **ids)
{
    SDL_Log("Android_JNI_GetTouchDeviceIds------");
    return 0;
}

/* sets the mSeparateMouseAndTouch field */
void Android_JNI_SetSeparateMouseAndTouch(SDL_bool new_value)
{
    SDL_Log("Android_JNI_SetSeparateMouseAndTouch------");
}

void Android_JNI_PollInputDevices(void)
{
    // SDL_Log("Android_JNI_PollInputDevices------");
}

void Android_JNI_PollHapticDevices(void)
{
    SDL_Log("Android_JNI_PollHapticDevices------");
}

void Android_JNI_HapticRun(int device_id, int length)
{
    SDL_Log("Android_JNI_HapticRun------");
}


/* See SDLActivity.java for constants. */
#define COMMAND_SET_KEEP_SCREEN_ON    5

/* sends message to be handled on the UI event dispatch thread */
int Android_JNI_SendMessage(int command, int param)
{
    SDL_Log("Android_JNI_SendMessage------");
    return 0;
}

void Android_JNI_SuspendScreenSaver(SDL_bool suspend)
{
    SDL_Log("Android_JNI_SuspendScreenSaver------");
}

void Android_JNI_ShowTextInput(SDL_Rect *inputRect)
{
    SDL_Log("Android_JNI_ShowTextInput------");
}

void Android_JNI_HideTextInput(void)
{
	SDL_Log("Android_JNI_HideTextInput------");
}

SDL_bool Android_JNI_IsScreenKeyboardShown()
{
    SDL_Log("Android_JNI_IsScreenKeyboardShown------");
	return SDL_FALSE;
}


int Android_JNI_ShowMessageBox(const SDL_MessageBoxData *messageboxdata, int *buttonid)
{
	SDL_Log("Android_JNI_ShowMessageBox------");
	return 0;
}

/*
//////////////////////////////////////////////////////////////////////////////
//
// Functions exposed to SDL applications in SDL_system.h
//////////////////////////////////////////////////////////////////////////////
*/

void *SDL_AndroidGetJNIEnv(void)
{
	SDL_Log("---SDL_AndroidGetJNIEnv---");
    return NULL;
}

void *SDL_AndroidGetActivity(void)
{
    SDL_Log("---SDL_AndroidGetActivity---");
    return NULL;
}

SDL_bool SDL_IsAndroidTV(void)
{
	SDL_Log("---SDL_IsAndroidTV---");
    return SDL_FALSE;
}

const char * SDL_AndroidGetInternalStoragePath(void)
{
    static char *s_AndroidInternalFilesPath = NULL;

	SDL_Log("SDL_AndroidGetInternalStoragePath, E");
    if (!s_AndroidInternalFilesPath) {
		kOS_osinfo info;
		sys2_get_osinfo(&info);
		s_AndroidInternalFilesPath = SDL_strdup(info.res_path);
		SDL_Log("SDL_AndroidGetInternalStoragePath, s_AndroidInternalFilesPath: %s", s_AndroidInternalFilesPath);
    }
	SDL_Log("SDL_AndroidGetInternalStoragePath, X");
    return s_AndroidInternalFilesPath;
}

int SDL_AndroidGetExternalStorageState(void)
{
	SDL_Log("---SDL_AndroidGetExternalStorageState---");
    return 0;
}

const char * SDL_AndroidGetExternalStoragePath(void)
{
    static char *s_AndroidExternalFilesPath = NULL;

	SDL_Log("SDL_AndroidGetExternalStoragePath, E");
    if (!s_AndroidExternalFilesPath) {
        kOS_osinfo info;
		sys2_get_osinfo(&info);
		s_AndroidExternalFilesPath = SDL_strdup(info.userdata_path);
		SDL_Log("SDL_AndroidGetExternalStoragePath, s_AndroidExternalFilesPath: %s", s_AndroidExternalFilesPath);
    }
	SDL_Log("SDL_AndroidGetExternalStoragePath, X");
    return s_AndroidExternalFilesPath;
}

void Android_JNI_GetManifestEnvironmentVariables(void)
{
	// SDL_Log("Request to get environment variables before JNI is ready");
}

jclass Android_JNI_GetActivityClass(void)
{
    return NULL;
}

JavaVM* Android_JNI_GetJavaVM(void)
{
	return NULL;
}

jobject Android_JNI_GetActivityObject(void)
{
	SDL_Log("---Android_JNI_GetActivityObject---");
	return NULL;
}

SDL_bool Android_JNI_IsDirectory(const char* dir)
{
	SDL_Log("Android_JNI_IsDirectory------");
	return SDL_TRUE;
}

SDL_bool Android_JNI_IsFile(const char* file)
{
	SDL_Log("Android_JNI_IsFile------");
	return SDL_TRUE;
}

int64_t Android_JNI_getLength64(char const* file)
{
	SDL_Log("Android_JNI_getLength64------");
	return 0;
}

typedef struct {
	char* data;
	int length;
	int pos;
} SDL_AndroidDIR;

SDL_DIR* Android_JNI_OpenDir(char const* dir)
{
	SDL_Log("Android_JNI_OpenDir------");
	return NULL;
}

SDL_dirent2* Android_JNI_ReadDir(SDL_DIR* dir)
{
	SDL_Log("Android_JNI_ReadDir------");
	return NULL;
}

int Android_JNI_CloseDir(SDL_DIR* dir)
{
	SDL_Log("Android_JNI_CloseDir------");
	return 0;
}

SDL_bool Android_JNI_GetStat(const char* name, SDL_dirent* stat)
{
	SDL_Log("Android_JNI_GetStat------");
	return SDL_TRUE;
}

typedef struct
{
	char* data;
	int* pos;
} SDL_AndroidPhoto;

SDL_Photo* Android_JNI_OpenPhoto(char const* dir)
{
	SDL_Log("Android_JNI_OpenPhoto------");
	return NULL;
}

void* Android_JNI_ReadPhoto(SDL_Photo* photos, const int index, const int width, const int height)
{
	SDL_Log("Android_JNI_ReadPhoto------");
	return NULL;
}

int Android_JNI_ClosePhoto(SDL_Photo* photos)
{
	SDL_Log("Android_JNI_ClosePhoto------");
	return 0;
}

#ifndef posix_mku32
	#define posix_mku32(l, h)		((uint32_t)(((uint16_t)(l)) | ((uint32_t)((uint16_t)(h))) << 16))
#endif

#ifndef posix_mku16
	#define posix_mku16(l, h)		((uint16_t)(((uint8_t)(l)) | ((uint16_t)((uint8_t)(h))) << 8))
#endif

SDL_bool Android_JNI_ReadCard(SDL_IDCardInfo* info)
{
	SDL_Log("Android_JNI_ReadCard------");
	return SDL_FALSE;
}

void Android_JNI_SetProperty(const char* key, const char* value)
{
	SDL_Log("Android_JNI_SetProperty------");
}

void Android_JNI_Reboot()
{
	SDL_Log("Android_JNI_Reboot------");
}

void Android_JNI_SetAppType(int type)
{
	SDL_Log("Android_JNI_SetAppType------");
}

SDL_bool Android_JNI_SetTime(int year, int month, int day, int hour, int minute, int second)
{
	SDL_Log("Android_JNI_SetTime------");
	return SDL_TRUE;
}

void Android_JNI_UpdateApp(const char* package)
{
	SDL_Log("Android_JNI_UpdateApp------");
	sys2_run_app(package);
}

void Android_JNI_GetOsInfo(SDL_OsInfo* info)
{
	SDL_Log("Android_JNI_GetOsInfo------");
	if (!info) {
		return;
	}
	SDL_memset(info, 0, sizeof(SDL_OsInfo));

	// manufacturer
	SDL_strlcpy(info->manufacturer, "rockchip", sizeof(info->manufacturer));

	// display
	SDL_strlcpy(info->display, "eng.leagor.", sizeof(info->display));
}

SDL_bool Android_JNI_PrintText(const uint8_t* bytes, int size)
{
	SDL_Log("Android_JNI_PrintText------");
	return SDL_FALSE;
}

void Android_EnableRelay(SDL_bool enable)
{
	SDL_Log("Android_JNI_EnableRelay------");
}

void Android_TurnOnFlashlight(SDL_LightColor color)
{
	SDL_Log("Android_TurnOnFlashlight------");
}

void Android_TurnOnScreen(SDL_bool on)
{
	SDL_Log("Android_TurnOnScreen------");
}

char* Android_JNI_CreateGUID(SDL_bool line)
{
	int len = 40;
	char* result = (char*)SDL_malloc(len);
	SDL_memset(result, 0, len);
	if (line) {
		const char* example = "c68a303e-680d-4239-bbe2-68743a945b41";
		SDL_memcpy(result, example, SDL_strlen(example));
	} else {
		const char* example = "c68a303e680d4239bbe268743a945b41";
		SDL_memcpy(result, example, SDL_strlen(example));
	}
	return result;
}

int Android_GetDpiScale()
{
	SDL_Log("------Android_GetDpiScale------");
	return 2;
}

int Android_GetStatusBarHeight()
{
	SDL_Log("------Android_GetStatusBarHeight------");
	int result = 32 * Android_GetDpiScale();

	Android_SetScreenResolution(1920, 1080 - result, SDL_PIXELFORMAT_RGB565, 127.00);
	return result;
}

void Android_ShowStatusBar(SDL_bool visible)
{
	SDL_Log("------Android_ShowStatusBar(%s)------", visible? "true": "false");
}

void Android_EnableStatusBar(SDL_bool enable)
{
	SDL_Log("------Android_EnableStatusBar(%s)------", enable? "true": "false");
}

// ble behaver
void Android_JNI_BleScanPeripherals(const char* uuid)
{
	SDL_Log("Android_JNI_BleScanPeripherals------");
}

void Android_JNI_BleStopScanPeripherals()
{
	SDL_Log("Android_JNI_BleStopScanPeripherals------");
}

jobject Android_JNI_BleInitialize()
{
	SDL_Log("Android_JNI_BleInitialize------");
	return NULL;
}

#endif /* __ANDROID__ */

/* vi: set ts=4 sw=4 expandtab: */

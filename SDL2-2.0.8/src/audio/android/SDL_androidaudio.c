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

#ifndef _KOS
#if SDL_AUDIO_DRIVER_ANDROID
/* Output audio to Android */

#include "SDL_assert.h"
#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "SDL_androidaudio.h"

#include "../../core/android/SDL_android.h"
#include "../../events/SDL_events_c.h"
#include "SDL_hints.h"

#include <android/log.h>
#include "SDL_timer.h"
// Base practical experience, there is a jack when start.
static SDL_bool capture_require_slice = SDL_FALSE;

static SDL_AudioDevice* audioDevice = NULL;
static SDL_AudioDevice* captureDevice = NULL;

static int
ANDROIDAUDIO_OpenDevice(_THIS, void *handle, const char *devname, int iscapture)
{
    SDL_AudioFormat test_format;

    SDL_assert((captureDevice == NULL) || !iscapture);
    SDL_assert((audioDevice == NULL) || iscapture);

    if (iscapture) {
        captureDevice = this;
		capture_require_slice = SDL_TRUE;
    } else {
        audioDevice = this;
    }

    this->hidden = (struct SDL_PrivateAudioData *) SDL_calloc(1, (sizeof *this->hidden));
    if (this->hidden == NULL) {
        return SDL_OutOfMemory();
    }

    test_format = SDL_FirstAudioFormat(this->spec.format);
    while (test_format != 0) { /* no "UNKNOWN" constant */
        if ((test_format == AUDIO_U8) || (test_format == AUDIO_S16LSB)) {
            this->spec.format = test_format;
            break;
        }
        test_format = SDL_NextAudioFormat();
    }

    if (test_format == 0) {
        /* Didn't find a compatible format :( */
        return SDL_SetError("No compatible audio format!");
    }

    if (this->spec.channels > 1) {
        this->spec.channels = 2;
    } else {
        this->spec.channels = 1;
    }

    if (this->spec.freq < 8000) {
        this->spec.freq = 8000;
    }
    if (this->spec.freq > 48000) {
        this->spec.freq = 48000;
    }

    /* TODO: pass in/return a (Java) device ID */
    this->spec.samples = Android_JNI_OpenAudioDevice(iscapture, this->spec.freq, this->spec.format == AUDIO_U8 ? 0 : 1, this->spec.channels, this->spec.samples);

    if (this->spec.samples == 0) {
        /* Init failed? */
        return SDL_SetError("Java-side initialization failed!");
    }

    SDL_CalculateAudioSpec(&this->spec);

    return 0;
}

static void
ANDROIDAUDIO_PlayDevice(_THIS)
{
    Android_JNI_WriteAudioBuffer();
}

static Uint8 *
ANDROIDAUDIO_GetDeviceBuf(_THIS)
{
    return Android_JNI_GetAudioBuffer();
}

static int
ANDROIDAUDIO_CaptureFromDevice(_THIS, void *buffer, int buflen)
{
    return Android_JNI_CaptureAudioBuffer(buffer, buflen);
}

static void
ANDROIDAUDIO_FlushCapture(_THIS)
{
    Android_JNI_FlushCapturedAudio();
}

#ifdef ANDROID
#include <android/log.h>
#define posix_print(format, ...) __android_log_print(ANDROID_LOG_INFO, "SDL", format, ##__VA_ARGS__)
#elif defined(__APPLE__) // mac os x/ios
#define posix_print(format, ...) fprintf(stderr, format, ##__VA_ARGS__) 
#else
#define posix_print(format, ...)
#endif

static void
ANDROIDAUDIO_CloseDevice(_THIS)
{
	posix_print("AndroidAUD_CloseDevice\n");

    /* At this point SDL_CloseAudioDevice via close_audio_device took care of terminating the audio thread
       so it's safe to terminate the Java side buffer and AudioTrack
     */
    Android_JNI_CloseAudioDevice(this->iscapture);
    if (this->iscapture) {
        SDL_assert(captureDevice == this);
        captureDevice = NULL;
    } else {
        SDL_assert(audioDevice == this);
        audioDevice = NULL;
    }
    SDL_free(this->hidden);
}

static jboolean nativeFillAudioPlayout(JNIEnv* env, jarray buffer, jboolean screen_on, SDL_bool is16Bit)
{
	SDL_AudioDevice* this = audioDevice;

	if (didbackground && sdl_apphandlers.background_handler) {
		sdl_apphandlers.background_handler(screen_on? SDL_TRUE: SDL_FALSE, sdl_apphandlers.background_param);
	}

	if (!SDL_AtomicGet(&this->enabled) || SDL_AtomicGet(&this->paused)) {
		return JNI_FALSE;
	}

	// here, I want to reduce copy, but fail! Hope someone can do it.
	// jboolean isCopy = JNI_FALSE;
	// void* bufferPinned = (*env)->GetShortArrayElements(env, buffer, &isCopy);
	void* bufferPinned;
	int length;

	if (is16Bit) {
		bufferPinned = (*env)->GetShortArrayElements(env, (jshortArray)buffer, NULL);
		length = (*env)->GetArrayLength(env, (jshortArray)buffer) * 2;
	} else {
		bufferPinned = (*env)->GetByteArrayElements(env, (jbyteArray)buffer, NULL);
		length = (*env)->GetArrayLength(env, (jbyteArray)buffer);
	}

	/* Only do anything if audio is enabled and not paused */
	static Uint32 last_ticks = 0;
	Uint32 now_ticks = SDL_GetTicks();
	if (now_ticks - last_ticks >= 5000) {
		posix_print("nativeFillAudioPlayout, bufferPinned: %p, length: %i, is16Bit: %s, paused: %s, screen_on: %s\n", bufferPinned, length, is16Bit? "true": "false", SDL_AtomicGet(&this->paused) ? "true" : "false", screen_on? "true": "false");
		last_ticks = now_ticks;
	}
	// Generate the data
	SDL_LockMutex(this->mixer_lock);
	(*this->spec.callback)(this->spec.userdata, bufferPinned, length);
	SDL_UnlockMutex(this->mixer_lock);

	if (is16Bit) {
		(*env)->ReleaseShortArrayElements(env, buffer, bufferPinned, 0);
	} else {
		(*env)->ReleaseByteArrayElements(env, buffer, bufferPinned, 0);
	}
	// (*env)->DeleteLocalRef(env, buffer);

	return JNI_TRUE;
}

/* The Audio playout native method */
JNIEXPORT jboolean JNICALL Java_org_libsdl_app_SDLService_nativeFillByteAudioPlayout(JNIEnv* env, jclass jcls, jarray buffer, jboolean screen_on)
{
	return nativeFillAudioPlayout(env, buffer, screen_on, SDL_FALSE);
}

JNIEXPORT jboolean JNICALL Java_org_libsdl_app_SDLService_nativeFillShortAudioPlayout(JNIEnv* env, jclass jcls, jarray buffer, jboolean screen_on)
{
	return nativeFillAudioPlayout(env, buffer, screen_on, SDL_TRUE);
}

static jboolean nativeFillAudioCapture(JNIEnv* env, jarray buffer, SDL_bool is16Bit)
{
	SDL_AudioDevice* this = captureDevice;

	if (!SDL_AtomicGet(&this->enabled) || SDL_AtomicGet(&this->paused)) {
		return JNI_FALSE;
	}

	// here, I want to reduce copy, but fail! Hope someone can do it.
	// jboolean isCopy = JNI_FALSE;
	// void* bufferPinned = (*env)->GetShortArrayElements(env, buffer, &isCopy);
	void* bufferPinned;
	int length;

	if (is16Bit) {
		bufferPinned = (*env)->GetShortArrayElements(env, (jshortArray)buffer, NULL);
		length = (*env)->GetArrayLength(env, (jshortArray)buffer) * 2;
	} else {
		bufferPinned = (*env)->GetByteArrayElements(env, (jbyteArray)buffer, NULL);
		length = (*env)->GetArrayLength(env, (jbyteArray)buffer);
	}

	/* Only do anything if audio is enabled and not paused */
	static Uint32 last_ticks = 0;
	Uint32 now_ticks = SDL_GetTicks();
	if (now_ticks - last_ticks >= 5000) {
		posix_print("nativeFillAudioCapture, bufferPinned: %p, length: %i, is16Bit: %s, paused: %s\n", bufferPinned, length, is16Bit? "true": "false", SDL_AtomicGet(&this->paused) ? "true" : "false");
		last_ticks = now_ticks;
	}
	// Generate the data
	SDL_LockMutex(this->mixer_lock);
	// call callback
	if (capture_require_slice) {
		SDL_memset(bufferPinned, this->spec.silence, length);
		capture_require_slice = SDL_FALSE;
	}
	(*this->spec.callback)(this->spec.userdata, bufferPinned, length);
	SDL_UnlockMutex(this->mixer_lock);

	if (is16Bit) {
		(*env)->ReleaseShortArrayElements(env, buffer, bufferPinned, JNI_ABORT);
	} else {
		(*env)->ReleaseByteArrayElements(env, buffer, bufferPinned, JNI_ABORT);
	}

	return JNI_TRUE;
}

/* The Audio Capture native */
JNIEXPORT jboolean JNICALL Java_org_libsdl_app_SDLService_nativeFillShortAudioCapture(JNIEnv* env, jclass jcls, jarray buffer)
{
	return nativeFillAudioCapture(env, buffer, SDL_TRUE);
}

static int
ANDROIDAUDIO_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->OpenDevice = ANDROIDAUDIO_OpenDevice;
    impl->PlayDevice = ANDROIDAUDIO_PlayDevice;
    impl->GetDeviceBuf = ANDROIDAUDIO_GetDeviceBuf;
    impl->CloseDevice = ANDROIDAUDIO_CloseDevice;
    impl->CaptureFromDevice = ANDROIDAUDIO_CaptureFromDevice;
    impl->FlushCapture = ANDROIDAUDIO_FlushCapture;

    /* and the capabilities */
    impl->HasCaptureSupport = SDL_TRUE;
    impl->OnlyHasDefaultOutputDevice = 1;
    impl->OnlyHasDefaultCaptureDevice = 1;

	impl->ProvidesOwnCallbackThread = 1;

    return 1;   /* this audio target is available. */
}

AudioBootStrap ANDROIDAUDIO_bootstrap = {
    "android", "SDL Android audio driver", ANDROIDAUDIO_Init, 0
};

/* Pause (block) all non already paused audio devices by taking their mixer lock */
void ANDROIDAUDIO_PauseDevices(void)
{
	const char* hint = SDL_GetHint(SDL_HINT_BACKGROUND_AUDIO);
    /* TODO: Handle multiple devices? */
    struct SDL_PrivateAudioData *private;
    if(audioDevice != NULL && audioDevice->hidden != NULL) {
        private = (struct SDL_PrivateAudioData *) audioDevice->hidden;
        if (SDL_AtomicGet(&audioDevice->paused)) {
            /* The device is already paused, leave it alone */
            private->resume = SDL_FALSE;
        }
        else if (hint && hint[0] == '0') {
            SDL_LockMutex(audioDevice->mixer_lock);
            SDL_AtomicSet(&audioDevice->paused, 1);
            private->resume = SDL_TRUE;
        }
    }
}

/* Resume (unblock) all non already paused audio devices by releasing their mixer lock */
void ANDROIDAUDIO_ResumeDevices(void)
{
    /* TODO: Handle multiple devices? */
    struct SDL_PrivateAudioData *private;
    if(audioDevice != NULL && audioDevice->hidden != NULL) {
        private = (struct SDL_PrivateAudioData *) audioDevice->hidden;
        if (private->resume) {
            SDL_AtomicSet(&audioDevice->paused, 0);
            private->resume = SDL_FALSE;
            SDL_UnlockMutex(audioDevice->mixer_lock);
        }
    }

    if(captureDevice != NULL && captureDevice->hidden != NULL) {
        private = (struct SDL_PrivateAudioData *) captureDevice->hidden;
        if (private->resume) {
            SDL_AtomicSet(&captureDevice->paused, 0);
            private->resume = SDL_FALSE;
            SDL_UnlockMutex(captureDevice->mixer_lock);
        }
    }
}

#else 

void ANDROIDAUDIO_ResumeDevices(void) {}
void ANDROIDAUDIO_PauseDevices(void) {}

#endif /* SDL_AUDIO_DRIVER_ANDROID */
#endif // _KOS

/* vi: set ts=4 sw=4 expandtab: */


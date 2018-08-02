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
#ifdef __ANDROID__
#include "../../SDL_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <EGL/eglplatform.h>
#include <android/native_window_jni.h>

#include "SDL_rect.h"

// Interface from the SDL library into the Android Java activity
extern SDL_bool Android_JNI_GetAccelerometerValues(float values[3]);

// Audio support
extern int Android_JNI_OpenAudioDevice(int iscapture, int sampleRate, int is16Bit, int channelCount, int desiredBufferFrames);
extern void* Android_JNI_GetAudioBuffer(void);
extern void Android_JNI_WriteAudioBuffer(void);
extern void Android_JNI_CloseAudioDevice(const int iscapture);

// Runtime support
void Android_nativeQuit();

// File support
#include "SDL_rwops.h"

// Threads
#include <jni.h>
/* 2017.10.10 - RJS - JavaVM removal.
JNIEnv *Android_JNI_GetEnv(void);
*/
int Android_JNI_SetupThread(void);

#ifdef __cplusplus
}
#endif
#endif

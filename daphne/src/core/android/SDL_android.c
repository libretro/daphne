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
#include "../../SDL_internal.h"
#include "SDL_stdinc.h"
#include "SDL_assert.h"
#include "SDL_hints.h"
#include "SDL_log.h"

#ifdef __ANDROID__

#include "SDL_system.h"
#include "SDL_android.h"
#include <EGL/egl.h>

#include "../../events/SDL_events_c.h"
#include "../../video/android/SDL_androidkeyboard.h"
#include "../../video/android/SDL_androidmouse.h"
#include "../../video/android/SDL_androidtouch.h"
#include "../../video/android/SDL_androidvideo.h"
#include "../../video/android/SDL_androidwindow.h"
#include "../../joystick/android/SDL_sysjoystick_c.h"
#include "../../../libretro/libretro_daphne.h"

#include <android/log.h>
#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include "../../../libretro/libretro.h"
#include "../../../main_android.h"

static void Android_JNI_ThreadDestroyed(void*);
/* 2017.10.10 - RJS - JavaVM removal.
static JavaVM* Android_JNI_GetVM(void);
*/

#include <jni.h>
#include <android/log.h>

//******************************************************************************
//******************************************************************************
static pthread_key_t mThreadKey = PTHREAD_KEYS_MAX;

// 2017.02.09 - RJS - init NULL
// static JavaVM* mJavaVM = NULL;

/* Accelerometer data storage */
static float fLastAccelerometer[3];
static SDL_bool bHasNewData;

//*******************************************************************************
//*******************************************************************************
JNIEXPORT void JNICALL SDL_Android_Init(JNIEnv* mEnv, jclass cls)
{
	LOGI("daphne-libretro: In SDL_Android_Init, top of routine.");

	if (pthread_key_create(&mThreadKey, Android_JNI_ThreadDestroyed) != 0)
	{
		LOGI("daphne-libretro: In SDL_Android_Init, ERROR initializing pthread_key.");
	}

	Android_JNI_SetupThread();

	// 2017.09.20 - RJS - Is this needed anymore as we aren't binding any JNI classes.
	// mActivityClass = (jclass)((*mEnv)->NewGlobalRef(mEnv, cls));

	// The whole point of running this routine is to load needed java bits, most are handled is some way through
	// RA.  Most of these routines need to be rewritten for RA since we don't really have our own Java routines.
	// Right now I would just return here and doc the rest of this routine out knowing all these will need to be
	// handled elsewhere like pollInput is already handled by RA, same with audio.  If not already open see
	// SeparateEventsHintWatcher, it's where we began.
	// 2017.09.20 - RJS - Routines removed, please see Daphne source if these are needed.

	LOGI("daphne-libretro: In SDL_Android_Init, bottom of routine.");
}

//*******************************************************************************
//*******************************************************************************
// Window resizing:	onNativeResize				-> Android_SetScreenResolution(width, height, format, rate);
// Pad down:		onNativePadDown				-> Android_OnPadDown(device_id, keycode);
// Pad up:			onNativePadUp				-> Android_OnPadUp(device_id, keycode);
// Joystick:		onNativeJoy					-> Android_OnJoy(device_id, axis, value);
// Hat:				onNativeHat					-> Android_OnHat(device_id, hat_id, x, y);
// Add Joy:			nativeAddJoystick			-> Android_AddJoystick(device_id, name, (SDL_bool) is_accelerometer, nbuttons, naxes, nhats, nballs);
// Remove Joy:		nativeRemoveJoystick		-> Android_RemoveJoystick(device_id);
// Surface changed:	onNativeSurfaceChanged		-> see original source (eventually calls SDL_EGL_CreateSurface)
// Surface destroy:	onNativeSurfaceDestroyed	-> see original source (eventually calls SDL_EGL_DestroySurface)
// Key down:		onNativeKeyDown				-> Android_OnKeyDown(keycode);
// Key up:			onNativeKeyUp				-> Android_OnKeyUp(keycode);
// KB focus lost:	onNativeKeyboardFocusLost	-> SDL_StopTextInput();
// Touch:			onNativeTouch				-> Android_OnTouch(touch_device_id_in, pointer_finger_id_in, action, x, y, p);
// Mouse:			onNativeMouse				-> Android_OnMouse(button, action, x, y);
// Accel:			onNativeAccel				-> fills fLastAccelerometer[3], sets bHasNewData
// Low memory:		nativeLowMemory				-> SDL_SendAppEvent(SDL_APP_LOWMEMORY);

// 2017.09.22 - RJS - I don't think nativeQuit/Pause/Resume are useable in a core
// but could be useful in the future if RA get new features.  None have been
// debugged yet, so they are mostly for reference - for instance window events are
// sent in onPause, there are no "windows" in this core, however, that event may trigger
// an important shutdown sequence (so leaving them in).

//*******************************************************************************
//*******************************************************************************
void Android_nativeQuit()
{
	LOGI("daphne-libretro: In Android_nativeQuit, top of routine.");

	// Discard previous events. The user should have handled state storage
	// in SDL_APP_WILLENTERBACKGROUND. After nativeQuit() is called, no
	// events other than SDL_QUIT and SDL_APP_TERMINATING should fire.
	SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

	// Inject a SDL_QUIT event
	SDL_SendQuit();
	SDL_SendAppEvent(SDL_APP_TERMINATING);

	// Resume the event loop so that the app can catch SDL_QUIT which
	// should now be the top event in the event queue.
	if (!SDL_SemValue(Android_ResumeSem)) SDL_SemPost(Android_ResumeSem);
	
	// I'm not sure, actually I know this is the cleanest but it's working.  No egregious
	// errors, all other threads are closed, and this thread is already cleaned up.
	// 2018.02.05 - RJS - nNeed to remove exit since this is no longer a standalone but a core.
	// exit(0);
}

//*******************************************************************************
//*******************************************************************************
void Android_nativePause()
{
	LOGI("daphne-libretro: In Android_nativePause, top of routine.");

	SDL_SendAppEvent(SDL_APP_WILLENTERBACKGROUND);
	SDL_SendAppEvent(SDL_APP_DIDENTERBACKGROUND);

	if (Android_Window)
	{
		SDL_SendWindowEvent(Android_Window, SDL_WINDOWEVENT_FOCUS_LOST, 0, 0);
		SDL_SendWindowEvent(Android_Window, SDL_WINDOWEVENT_MINIMIZED, 0, 0);

		// *After* sending the relevant events, signal the pause semaphore 
		// so the event loop knows to pause and (optionally) block itself.
        if (!SDL_SemValue(Android_PauseSem)) SDL_SemPost(Android_PauseSem);
    }
}

//*******************************************************************************
//*******************************************************************************
void Android_nativeResume()
{
	LOGI("daphne-libretro: In Android_nativeResume, top of routine.");

	SDL_SendAppEvent(SDL_APP_WILLENTERFOREGROUND);
	SDL_SendAppEvent(SDL_APP_DIDENTERFOREGROUND);

	if (Android_Window)
	{
		SDL_SendWindowEvent(Android_Window, SDL_WINDOWEVENT_FOCUS_GAINED, 0, 0);
		SDL_SendWindowEvent(Android_Window, SDL_WINDOWEVENT_RESTORED, 0, 0);
		
		// Signal the resume semaphore so the event loop knows to resume and restore the GL Context
		// We can't restore the GL Context here because it needs to be done on the SDL main thread
		// and this function will be called from the Java thread instead.
		if (!SDL_SemValue(Android_ResumeSem)) SDL_SemPost(Android_ResumeSem);
    }
}

// Low memory:		nativeLowMemory				-> SDL_SendAppEvent(SDL_APP_LOWMEMORY);
// Commit text:		nativeCommitText			-> SDL_SendKeyboardText(utftext);
// Set composing:	nativeSetComposingText		-> SDL_SendEditingText(utftext, 0, 0);
// Get hint:		nativeGetHint				-> SDL_GetHint(utfname);

// REMOVED: static struct LocalReferenceHolder LocalReferenceHolder_Setup(const char *func)
// REMOVED: static SDL_bool LocalReferenceHolder_Init(struct LocalReferenceHolder *refholder, JNIEnv *env)
// REMOVED: static void LocalReferenceHolder_Cleanup(struct LocalReferenceHolder *refholder)
// REMOVED: static SDL_bool LocalReferenceHolder_IsActive(void)
// REMOVED: ANativeWindow* Android_JNI_GetNativeWindow(void) { return NULL; }
// REMOVED: void Android_JNI_SetActivityTitle(const char *title)

//*******************************************************************************
//*******************************************************************************
SDL_bool Android_JNI_GetAccelerometerValues(float values[3])
{
	SDL_bool retval = SDL_FALSE;

	if (bHasNewData) {
		int i;
		for (i = 0; i < 3; ++i)
		{
			values[i] = fLastAccelerometer[i];
		}
		bHasNewData = SDL_FALSE;
		retval = SDL_TRUE;
	}

	return retval;
}

// RJS - ADD 2017.07.20 - Since we aren't getting the JavaVM or JNIEnv passed from LR we need to 
// do it ourselves.  This is taken from some Android source, DdmConnection::start(const char* name).
// However, this is not an approved way to get the VM but since they're doing it should be OK for
// awhile.  Otherwise, they'll come up with a new method and that will need to go here.  Or just get
// passed down from LR.  Anyways, here goes.
// So we can't do this anymore: https://developer.android.com/about/versions/nougat/android-7.0-changes.html#ndk
// Thanks Google, probably need to pass the actual VM down from LR.  This will be fun I'm sure.
// RJS - 2017.09.20 - Code to do above removed, was just below.  Keeping this log for reference.

//*******************************************************************************
//*******************************************************************************
static void Android_JNI_ThreadDestroyed(void* value)
{
	/* 2017.10.10 - RJS - JavaVM removal.
	JavaVM * javaVM = Android_JNI_GetVM();
	if (!javaVM)
	{
		LOGI("daphne-libretro: In Android_JNI_ThreadDestroyed, javaVM is NULL, returning.");
		return;
	}

	// The thread is being destroyed, detach it from the Java VM and set the mThreadKey value to NULL as required.
	JNIEnv * env = (JNIEnv*) value;
	if (env)
	{
		LOGI("daphne-libretro: In Android_JNI_ThreadDestroyed, ThreadDestroyed(), tid: %d", gettid());
		(*javaVM)->DetachCurrentThread(javaVM);
		pthread_setspecific(mThreadKey, NULL);
	}
	*/

	pthread_setspecific(mThreadKey, NULL);
}

//******************************************************************************
//******************************************************************************
/* 2017.10.10 - RJS - JavaVM removal.
static JavaVM* Android_JNI_GetVM(void)
{
	if (!mJavaVM)
	{
		mJavaVM = retro_get_javavm();
		if (!mJavaVM)
		{
			LOGI("daphne-libretro: In Android_JNI_GetEnv, mJavaVM initialization FAILED, returning NULL.");
		}
	}

	return mJavaVM;
}
*/

//******************************************************************************
//******************************************************************************
/* 2017.10.10 - RJS - JavaVM removal.
JNIEnv* Android_JNI_GetEnv(void)
{
	JavaVM * javaVM = Android_JNI_GetVM();
	if (!javaVM) return NULL;

	// From http://developer.android.com/guide/practices/jni.html
	// All threads are Linux threads, scheduled by the kernel.
	// They're usually started from managed code (using Thread.start), but they can also be created elsewhere and then
	// attached to the JavaVM. For example, a thread started with pthread_create can be attached with the
	// JNI AttachCurrentThread or AttachCurrentThreadAsDaemon functions. Until a thread is attached, it has no JNIEnv,
	// and cannot make JNI calls.
	// Attaching a natively-created thread causes a java.lang.Thread object to be constructed and added to the "main"
	// ThreadGroup, making it visible to the debugger. Calling AttachCurrentThread on an already-attached thread
	// is a no-op.
	// Note: You can call this function any number of times for the same thread, there's no harm in it

	JNIEnv *env;
	int status = (*javaVM)->AttachCurrentThread(javaVM, &env, NULL);
	if (status < 0)
	{
		LOGI("daphne-libretro: In Android_JNI_GetEnv, attach current thread FAILED, returning NULL.  status: %d", status);
		return NULL;
	}

	// From http://developer.android.com/guide/practices/jni.html
	// Threads attached through JNI must call DetachCurrentThread before they exit. If coding this directly is awkward,
	// in Android 2.0 (Eclair) and higher you can use pthread_key_create to define a destructor function that will be
	// called before the thread exits, and call DetachCurrentThread from there. (Use that key with pthread_setspecific
	// to store the JNIEnv in thread-local-storage; that way it'll be passed into your destructor as the argument.)
	// Note: The destructor is not called unless the stored value is != NULL
	// Note: You can call this function any number of times for the same thread, there's no harm in it
	// (except for some lost CPU cycles)
	pthread_setspecific(mThreadKey, (void*)env);

	return env;
}
*/

//******************************************************************************
//******************************************************************************
int Android_JNI_SetupThread(void)
{
	if (pthread_setspecific(mThreadKey, NULL) == 0) return 1;
	/* 2017.10.10 - RJS - JavaVM removal.
	if (Android_JNI_GetEnv()) return 1;
	*/
	return 0;
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// Audio support
//******************************************************************************
//******************************************************************************
//******************************************************************************
#define AUDIO_BUFFER_AMOUNT 40
typedef enum
{
	AB_STATE_USEABLE,
	AB_STATE_FILLING,
	AB_STATE_WAITING,
	AB_STATE_STREAMING,
	AB_STATE_STREAMING_DONE,
	AB_STATE_AMOUNT,
	AB_STATE_NOTHING = -1
} AUDIO_BUFFER_STATE;

typedef struct
{
	AUDIO_BUFFER_STATE	buffer_state;
	int			audio_buffer_frames;
	int16_t *	audio_buffer_malloc;
	int16_t *	audio_buffer;
} AUDIO_BUFFER;

int g_ab_filling_queue	= -1;
int g_ab_waiting_top	= -1;
int g_ab_waiting_next	= 0;
int g_ab_waiting_queue[AUDIO_BUFFER_AMOUNT];

AUDIO_BUFFER g_ab[AUDIO_BUFFER_AMOUNT];

// #define LOGABSYS(STRIN) ((void)__android_log_print(ANDROID_LOG_INFO, "RA...CoreAB", "fb: %d\t\tw_top: %d\tw_next: %d\tw_q: [0]:%d\t[1]:%d\t[2]:%d\t[3]:%d\t Buffer: [0]:%d\t[1]:%d\t[2]:%d\t[3]:%d  "STRIN, g_ab_filling_queue, g_ab_waiting_top, g_ab_waiting_next, g_ab_waiting_queue[0], g_ab_waiting_queue[1], g_ab_waiting_queue[2], g_ab_waiting_queue[3], g_ab[0].buffer_state, g_ab[1].buffer_state, g_ab[2].buffer_state, g_ab[3].buffer_state))
#define LOGABSYS(STRIN)

int audioBufferSize = 0;						// This is the _malloc size.
jboolean audioBuffer16Bit = JNI_FALSE;			// Is audio buffer 16 bit.

// static void* audioBufferPinned = NULL;			// Keeping this in line with original code.  Not 100% sure it's unnecessary.
int16_t * audioBufferPinned_malloc = NULL;			// This must be the free'd buffer.
int16_t * audioBufferPinned = NULL;					// This is the above buffer 16bit aligned.

// ********************************************************************************************************************************
// ********************************************************************************************************************************
int16_t * get_ab_next_usable(int * ab_ndx)
{
	LOGABSYS("getUSABLE, top, before RECLAIM.");

	// Reclaim already rendered buffers.
	int i;
	for (i = 0; i < AUDIO_BUFFER_AMOUNT; i++)
	{
		if (g_ab[i].buffer_state == AB_STATE_STREAMING_DONE)
		{
			g_ab[i].buffer_state = AB_STATE_USEABLE;
		}
	}

	LOGABSYS("getUSABLE, after RECLAIM.");

	// Crawl all buffers looking for a STATE_USABLE buffer.
	int ab_next;
	for (ab_next = 0; ab_next < AUDIO_BUFFER_AMOUNT; ab_next++)
	{
		if (g_ab[ab_next].buffer_state == AB_STATE_USEABLE) break;
	}

	// We could consider a stall and wait for a usable audio buffer as one should be in the 
	// middle of streaming.  Though a better solution might be to allow more buffers.
	if ((ab_next >= AUDIO_BUFFER_AMOUNT) &&
		(g_ab_waiting_top == g_ab_waiting_next))
	{
		g_ab_waiting_next--;
		if (g_ab_waiting_next < 0) g_ab_waiting_next = AUDIO_BUFFER_AMOUNT - 1;
		ab_next = g_ab_waiting_next;
		g_ab[(g_ab_waiting_queue[ab_next])].buffer_state = AB_STATE_USEABLE;
		LOGABSYS("getUSABLE, NO USABLE FOUND, eating most recent from WAIT_Q!");
	}

	// Check for and deal with no more usable buffers.
	if (ab_next >= AUDIO_BUFFER_AMOUNT)
	{
		// This means we have no usable buffers.  Debug why, consider making VIDEO_BUFFER_AMOUNT larger, or
		// frame skip making the g_first_waiting USABLE and the next WAITING the g_first_waiting.  In the
		// initial implimentation were just going to log so possible errors can be attacked.
		LOGABSYS("getUSABLE, NO USEABLE BUFFERS, exiting with NULL!");
		if (ab_ndx != NULL) *ab_ndx = -1;
		return NULL;
	}

	// We are going to transition this found usable buffer to a buffer being STATE_FILLING.

	// Make sure and deal with a buffer already being filled which should never happen.
	if (g_ab_filling_queue >= 0)
	{
		// This means we are filling two buffers which should never happen.  In the
		// initial implimentation were just going to log so possible errors can be attacked.
		LOGABSYS("getUSABLE, FILLING TWO+ buffers, exiting with NULL!");
		if (ab_ndx != NULL) *ab_ndx = -1;
		return NULL;
	}

	// Change state to filling and add to the single entry filling queue.
	if (ab_ndx != NULL) *ab_ndx = ab_next;
	g_ab_filling_queue = ab_next;
	g_ab[ab_next].buffer_state = AB_STATE_FILLING;
	LOGABSYS("getUSABLE, bottom, found USABLE, moved to FILLING.");
	return(g_ab[ab_next].audio_buffer);
}

// ********************************************************************************************************************************
// ********************************************************************************************************************************
void set_ab_filling_done(int ab_ndx)
{
	LOGABSYS("setFILLING_DONE, top.");

	// Check the input ndx and make sure it's a filling queue.
	if (ab_ndx >= AUDIO_BUFFER_AMOUNT)
	{
		LOGABSYS("setFILLING_DONE, input ndx wrong, exiting!");
		return;
	}
	if (g_ab[ab_ndx].buffer_state != AB_STATE_FILLING)
	{
		LOGABSYS("setFILLING_DONE, FILLING buffer wrong state, exiting!");
		return;
	}

	// Clear the filling queue.
	g_ab_filling_queue = -1;
	LOGABSYS("setFILLING_DONE, CLEARING FILLING_Q.");

	// Move from filling queue to waiting queue.

	// Handle situation where the next pointer and top pointer are the same.  This denotes a
	// filled waiting buffer.  We'll try to reclaim the most recent buffer since we don't
	// want to do an atomic by reclaiming the least recent buffer as that should be being
	// picked up by the presentation thread.
	if (g_ab_waiting_top == g_ab_waiting_next)
	{
		LOGABSYS("setFILLING_DONE, WAITING_Q FILLED, need buffer, eating most recent!");
		g_ab_waiting_next--;
		if (g_ab_waiting_next < 0) g_ab_waiting_next = AUDIO_BUFFER_AMOUNT - 1;
	}

	// Calculate next waiting queue position.
	int ab_next_waiting_ndx = g_ab_waiting_next + 1;
	if (ab_next_waiting_ndx >= AUDIO_BUFFER_AMOUNT) ab_next_waiting_ndx = 0;
	if (ab_next_waiting_ndx == g_ab_waiting_top)
	{
		LOGABSYS("setFILLING_DONE, WAITING_Q now FILLED!");
	}

	// While we are letting the presentation thread handle g_vb_waiting_top, on first entry, we are not.
	if (g_ab_waiting_top < 0) g_ab_waiting_top = g_ab_waiting_next;

	// Put filled buffer into waiting list.
	g_ab[ab_ndx].buffer_state = AB_STATE_WAITING;
	g_ab_waiting_queue[g_ab_waiting_next] = ab_ndx;
	g_ab_waiting_next = ab_next_waiting_ndx;

	LOGABSYS("setFILLING_DONE, bottom.");
}

// ********************************************************************************************************************************
// ********************************************************************************************************************************
int16_t * get_ab_waiting(int * ab_ndx, int * ab_frames)
{
	LOGABSYS("getWAITING, top.");

	// Make sure soemthing is waiting.
	if (ab_ndx != NULL) *ab_ndx = -1;
	if (ab_frames != NULL) *ab_frames = 0;
	if (g_ab_waiting_top < 0)
	{
		LOGABSYS("getWAITING, nothing is WAITING, exiting NULL!");
		return NULL;
	}

	int ab_rendering_ndx = g_ab_waiting_top;
	int ab_new_top = g_ab_waiting_top + 1;
	if (ab_new_top >= AUDIO_BUFFER_AMOUNT) ab_new_top = 0;

	// Check if there is another buffer waiting.  If not don't increment to next buffer.  We'll
	// try to keep this one.
	if ((g_ab_waiting_queue[ab_new_top] >= 0) &&
		(g_ab[(g_ab_waiting_queue[ab_new_top])].buffer_state == AB_STATE_WAITING))
	{
		g_ab_waiting_top = ab_new_top;
		LOGABSYS("getWAITING, new top WAITING buffer.");
	} else {
		g_ab_waiting_top = -1;
		g_ab_waiting_next = 0;
		LOGABSYS("getWAITING, NO new top WAITING buffer.");
	}

	g_ab[(g_ab_waiting_queue[ab_rendering_ndx])].buffer_state = AB_STATE_STREAMING;

	int16_t * ab_rendering = g_ab[(g_ab_waiting_queue[ab_rendering_ndx])].audio_buffer;
	if (ab_frames != NULL) *ab_frames = g_ab[(g_ab_waiting_queue[ab_rendering_ndx])].audio_buffer_frames;
	if (ab_ndx != NULL) *ab_ndx = ab_rendering_ndx;

	LOGABSYS("getWAITING, bottom, valid RENDERING buffer found.");
	return(ab_rendering);
}

// ********************************************************************************************************************************
// ********************************************************************************************************************************
void set_ab_streaming_done(int ab_ndx)
{
	LOGABSYS("setSTREAMING_DONE, top.");

	if (ab_ndx < 0) return;

	int buffer_ndx = g_ab_waiting_queue[ab_ndx];
	if (buffer_ndx < 0)
	{
		LOGABSYS("setSTREAMING_DONE, given streaming buffer is INVALID!");
		return;
	}

	if (g_ab[buffer_ndx].buffer_state != AB_STATE_STREAMING)
	{
		LOGABSYS("setSTREAMING_DONE, given buffer is not in STREAMING state!");
		return;
	}

	g_ab[buffer_ndx].buffer_state = AB_STATE_STREAMING_DONE;
	LOGABSYS("setSTREAMING_DONE, bottom, buffer set to DONE.");

	// 2017.11.06 - RJS - This shouldn't be necessary, but make for easier debugging.
	g_ab_waiting_queue[ab_ndx] = -1;
}

//******************************************************************************
//******************************************************************************
int Android_JNI_OpenAudioDevice_NEW(int iscapture, int sampleRate, int is16Bit, int channelCount, int desiredBufferFrames)
{
	LOGI("daphne-libretro: In %s, top of routine, iscapture: %d  sampleRate: %d  is16Bit: %d  channelCount: %d  desiredBufferFrames: %d", __func__, iscapture, sampleRate, is16Bit, channelCount, desiredBufferFrames);

	if (iscapture)
	{
		LOGI("daphne-libretro: In %s, iscapture not handled, returning 0.", __func__);
		return 0;
	}

	if (!Android_JNI_SetupThread())
	{
		LOGI("daphne-libretro: In %s, failed to attach current thread, returning 0.", __func__);
		return 0;
	}

	int audioBufferStereo = channelCount > 1;
	audioBuffer16Bit = is16Bit;

	int audioBufferStereoMultiplier = 1;
	if (audioBufferStereo > 1) audioBufferStereoMultiplier = 2;

	if (is16Bit)	audioBufferSize = desiredBufferFrames * audioBufferStereoMultiplier * 2;
	else			audioBufferSize = desiredBufferFrames * audioBufferStereoMultiplier;
	
	int i;
	for (i = 0; i < AUDIO_BUFFER_AMOUNT; i++)
	{
		g_ab[i].buffer_state		= AB_STATE_USEABLE;
		g_ab[i].audio_buffer_frames	= 0;

		if (is16Bit)
		{
			g_ab[i].audio_buffer_malloc = (int16_t *)malloc(audioBufferSize + 16);
			int16_t * audioBufferLocal = (int16_t *)(((uintptr_t)g_ab[i].audio_buffer_malloc + 15) & ~(uintptr_t)0x0f);
			g_ab[i].audio_buffer = audioBufferLocal;
		} else {
			int8_t * audioBufferLocal = (int8_t *)malloc(audioBufferSize);
			g_ab[i].audio_buffer = (int16_t *)audioBufferLocal;
			g_ab[i].audio_buffer_malloc = g_ab[i].audio_buffer;
		}

		if (g_ab[i].audio_buffer_malloc == NULL)
		{
			g_ab[i].buffer_state = AB_STATE_NOTHING;
			LOGI("daphne-libretro: In %s, could not allocation audio buffer!  Line: %d", __func__, __LINE__);
		}

		g_ab_waiting_queue[i] = -1;
	}

	g_ab_filling_queue	= -1;
	g_ab_waiting_top	= -1;
	g_ab_waiting_next	= 0;

	// 2018.04.03 - RJS - The pinned buffer is used to write into from the stream.  Then this buffer is
	// passed to the device to play.  We are going to mimic the pinned buffer for writing into and then
	// the buffer system will act as the go between from pinned to device.  This will require a buffer
	// copy and, if needed, this is an obvious optimization where a buffer in the chain is given up for
	// the pinned buffer and then folded back in when given to the device.
	if (is16Bit)
	{
		audioBufferPinned_malloc = (int16_t *)malloc(audioBufferSize + 16);
		int16_t * audioBufferLocal = (int16_t *)(((uintptr_t)audioBufferPinned_malloc + 15) & ~(uintptr_t)0x0f);
		audioBufferPinned = audioBufferLocal;
	} else {
		int8_t * audioBufferLocal = (int8_t *)malloc(audioBufferSize);
		audioBufferPinned = (int16_t *)audioBufferLocal;
		audioBufferPinned_malloc = audioBufferPinned;
	}

	return desiredBufferFrames;
}

//******************************************************************************
//******************************************************************************
// Finally used in: RunAudio to get latest (next) buffer
// Used in : ANDROIDAUDIO_Init when it's pointed to by: impl->GetDeviceBuf
// Called from: ANDROIDAUDIO_GetDeviceBuf
void * Android_JNI_GetAudioBuffer_NEW(void)
{
	return (void *) audioBufferPinned;
}

//******************************************************************************
//******************************************************************************
// extern retro_audio_sample_batch_t	cb_audiosamplebatch;
// static int16_t * audioBuffer8bitTo16bit = NULL;

// Called from: ANDROIDAUDIO_PlayDevice
void Android_JNI_WriteAudioBuffer_NEW(void)
{
	int ab_ndx = -1;
	int16_t * audioBufferToUse = get_ab_next_usable(&ab_ndx);
	LOGI("In Android_JNI_WriteAudioBuffer, top of routine.  audioBufferToUse: %d  audioBuffer16Bit: %d", (int)audioBufferToUse, (int)audioBuffer16Bit);

	if (! audioBufferToUse)
	{
		LOGABSYS("In Android_JNI_WriteAudioBuffer, no next usable audio buffer.  Exiting.");
		return;
	}

	int audioBufferToUseFrames;
	if (!audioBuffer16Bit)	audioBufferToUseFrames = audioBufferSize;
	else					audioBufferToUseFrames = audioBufferSize / 2;

	if (!audioBuffer16Bit)
	{
		LOGI("In Android_JNI_WriteAudioBuffer, after audioBuffer16Bit (false) check.");
		LOGI("In Android_JNI_WriteAudioBuffer, before crunching buffer.  audioBufferSize: %d", (int)audioBufferSize);
		int i;
		int8_t * audioBufferCrawl = (int8_t *)audioBufferToUse;
		for (i = 0; i < audioBufferSize; i++)
		{
			int8_t audioData8		= (int8_t) (audioBufferPinned[i]);
			*audioBufferCrawl		= audioData8;
			*(audioBufferCrawl + 1)	= audioData8;
			audioBufferCrawl += 2;
		}
	} else {
		// 2018.04.03 - RJS - Filling from pinned buffer.
		memcpy(audioBufferToUse, audioBufferPinned, audioBufferSize);
	}

	set_ab_filling_done(ab_ndx);
	g_ab[ab_ndx].audio_buffer_frames = audioBufferToUseFrames;

	// 2018.04.02 - RJS - Just slamming this in.  I've lost any memory of what a "pinned" buffer is since we've only
	// had one for so long.  Thus I send you, pinned buffer, into the wild.  Be free.
	// audioBufferPinned = g_ab[ab_ndx].audio_buffer;

	// LOGI("In Android_JNI_WriteAudioBuffer, before cb_audiosamplebatch callback.  audioBufferTouse: %d  audioBufferToUseFrames: %d  cb: %d", (int)audioBufferToUse, (int)audioBufferToUseFrames, (int)cb_audiosamplebatch);
	// if (cb_audiosamplebatch) cb_audiosamplebatch(audioBufferToUse, audioBufferToUseFrames);
	// LOGI("In Android_JNI_WriteAudioBuffer, after cb_audiosamplebatch callback.  Exiting routine.");
}

//******************************************************************************
//******************************************************************************
void Android_JNI_CloseAudioDevice_NEW(const int iscapture)
{
	int i;
	for (i = 0; i < AUDIO_BUFFER_AMOUNT; i++)
	{
		if (g_ab[i].audio_buffer_malloc) free(g_ab[i].audio_buffer_malloc);
		g_ab[i].audio_buffer_malloc	= NULL;
		g_ab[i].audio_buffer		= NULL;
		g_ab[i].audio_buffer_frames	= 0;
		g_ab[i].buffer_state		= AB_STATE_USEABLE;
		// audioBufferPinned			= NULL;
	}

	audioBufferSize = 0;

	if (audioBufferPinned_malloc)
	{
		free(audioBufferPinned_malloc);
		audioBufferPinned_malloc	= NULL;
		audioBufferPinned			= NULL;
	}
}


//*********************
//***** ORIG CODE *****
//*********************

// 2017.08.16 - RJS - changing from Java to native malloc.  Keeping name for now.
int16_t * audioBuffer_malloc	= NULL;			// This must be the free'd buffer.
int16_t * audioBuffer			= NULL;			// This is the above buffer 16bit aligned.
// IN NEW AND ORIG int audioBufferSize				= 0;			// This is the _malloc size.
// IN NEW AND ORIG jboolean audioBuffer16Bit		= JNI_FALSE;	// Is audio buffer 16 bit.
static void* audioBufferPinned_ORIG	= NULL;			// Keeping this in line with original code.  Not 100% sure it's unnecessary.

// static jboolean captureBuffer16Bit	= JNI_FALSE;	// Not currently used, as capture is disabled, maybe permanently.
// static jobject captureBuffer		= NULL;				// Not currently used, as capture is disabled, maybe permanently.

// ******************************************************************************
// ******************************************************************************
int Android_JNI_OpenAudioDevice_ORIG(int iscapture, int sampleRate, int is16Bit, int channelCount, int desiredBufferFrames)
{
	LOGI("daphne-libretro: In %s, top of routine, iscapture: %d  sampleRate: %d  is16Bit: %d  channelCount: %d  desiredBufferFrames: %d", __func__, iscapture, sampleRate, is16Bit, channelCount, desiredBufferFrames);

	// 2017.08.16 - RJS - We aren't going to handle iscapture.  All iscapture references removed in this routine.
	if (iscapture)
	{
		LOGI("daphne-libretro: In %s, iscapture not handled, returning 0.", __func__);
		return 0;
	}

	if (!Android_JNI_SetupThread())
	{
		LOGI("daphne-libretro: In %s, failed to attach current thread, returning 0.", __func__);
		return 0;
	}

	int audioBufferStereo = channelCount > 1;
	audioBuffer16Bit = is16Bit;

	// 2017.09.20 - RJS - Ignore below comment, left for legacy.
	// 2017.08.14 - RJS - I can't find the either of these capture functions (openCapture or openAudio).  Not sure how this bit was developed.  Maybe I took
	// a bit of code that was in transition.  Either way I'm docing this stuff out - punt for later audio issues I'm sure.

	// 2017.08.16 - RJS - Changing audio buffers to malloc'd space.  I haven't yet searched through all the code, but
	// this seems prudent right now.  Not sure how compatible these audio buffers are to what RA needs.
	int audioBufferStereoMultiplier = 1;
	if (audioBufferStereo > 1) audioBufferStereoMultiplier = 2;

	if (is16Bit)
	{
		audioBufferSize = desiredBufferFrames * audioBufferStereoMultiplier * 2;
		audioBuffer_malloc = (int16_t *)malloc(audioBufferSize + 16);
		int16_t * audioBufferLocal = (int16_t *) (((uintptr_t)audioBuffer_malloc + 15) & ~(uintptr_t)0x0f);
		audioBuffer = audioBufferLocal;
	} else {
		audioBufferSize = desiredBufferFrames * audioBufferStereoMultiplier;
		int8_t * audioBufferLocal = (int8_t *)malloc(audioBufferSize);
		audioBuffer = (int16_t *)audioBufferLocal;
	}

	if (audioBuffer == NULL)
	{
		LOGI("daphne-libretro: In %s, could not allocation audio buffer!  Returning 0.  Line: %d", __func__, __LINE__);
		audioBufferSize = 0;
		return 0;
	}

	// 2017.09.20 - RJS- Ignore below comment, left for legacy.
	// 2017.08.16 - RJS - Not sure why using GetArrayLength is needed.  Maybe NewXxArray doesn't always guarentee size or something?
	// Changed Pinned to point to buffer.
	audioBufferPinned_ORIG = audioBuffer;

	return desiredBufferFrames;
}

// ******************************************************************************
// ******************************************************************************
void * Android_JNI_GetAudioBuffer_ORIG(void)
{
	return audioBufferPinned_ORIG;
}

// ******************************************************************************
// ******************************************************************************
extern retro_audio_sample_batch_t	cb_audiosamplebatch;
static int16_t * audioBuffer8bitTo16bit = NULL;

// Called from: ANDROIDAUDIO_PlayDevice
void Android_JNI_WriteAudioBuffer_ORIG(void)
{
	LOGI("In Android_JNI_WriteAudioBuffer, top of routine.  audioBuffer: %d  audioBuffer16Bit: %d", (int)audioBuffer, (int)audioBuffer16Bit);
	int16_t * audioBufferToUse	= audioBuffer;

	int audioBufferToUseFrames;
	if (!audioBuffer16Bit)	audioBufferToUseFrames = audioBufferSize;
	else					audioBufferToUseFrames = audioBufferSize / 2;

	if (!audioBufferToUse) return;

	// 2017.09.20 - RJS - Ignore next two comments, left for legacy.
	// 2017.08.16 - RJS - Since we aren't jacking around with Java arrays, Release isn't necessary.
	// 2017.08.17 - RJS - We're going to set a flag here saying the audio buffer is ready instead of just 
	// writing it now since we need to do that in retro_run using the audio callback.  Problem is that
	// this flag and when retro_run is ready could be just out of sync.  If so we may need to change SDL_RunAudio
	// to wait until written, is one idea.

	// 2017.09.20 - RJS - This is a bit of hackery that we might get away with on most platforms.  However, if needed
	// please see SDL_RunAudio > SDL_BufferQueueDrainCallback > dequeue_audio_from_device.  This is where the buffer
	// is written.
	if (!audioBuffer16Bit)
	{
		LOGI("In Android_JNI_WriteAudioBuffer, after audioBuffer16Bit (false) check.");
		if (!audioBuffer8bitTo16bit) audioBuffer8bitTo16bit = (int16_t *)malloc(audioBufferToUseFrames * 2);
		if (!audioBuffer8bitTo16bit) return;

		LOGI("In Android_JNI_WriteAudioBuffer, before crunching buffer.  audioBufferSize: %d", (int)audioBufferSize);
		int i;
		int8_t * audioBufferCrawl = (int8_t *) audioBuffer8bitTo16bit;
		for (i = 0; i < audioBufferSize; i++)
		{
			*audioBufferCrawl		= audioBufferToUse[i];
			*(audioBufferCrawl + 1)	= audioBufferToUse[i];
			audioBufferCrawl += 2;
		}

		audioBufferToUse = audioBuffer8bitTo16bit;
	}

	LOGI("In Android_JNI_WriteAudioBuffer, before cb_audiosamplebatch callback.  audioBufferTouse: %d  audioBufferToUseFrames: %d  cb: %d", (int)audioBufferToUse, (int)audioBufferToUseFrames, (int) cb_audiosamplebatch);
	// LOGI("daphne-libretro: In %s, writing AUDIO buffer.", __func__);
	// cb_audiosamplebatch == in_audiosamplebatch == RA:audio_driver_sample_batch
	if (cb_audiosamplebatch) cb_audiosamplebatch(audioBufferToUse, audioBufferToUseFrames);
	LOGI("In Android_JNI_WriteAudioBuffer, after cb_audiosamplebatch callback.  Exiting routine.");
}


// 2017.09.20 - RJS - removed, names kept for reference
// REMOVED: int Android_JNI_CaptureAudioBuffer(void *buffer, int buflen)
// REMOVED: void Android_JNI_FlushCapturedAudio(void)

// ******************************************************************************
// ******************************************************************************
void Android_JNI_CloseAudioDevice_ORIG(const int iscapture)
{
	// 2017.09.20 - RJS- iscapture functionality removed

	if (audioBuffer_malloc)
	{
		free(audioBuffer_malloc);
		audioBufferSize		= 0;
		audioBuffer_malloc	= NULL;
		audioBuffer			= NULL;
		audioBufferPinned_ORIG	= NULL;
	}
}

// REMOVED: static SDL_bool Android_JNI_ExceptionOccurred(SDL_bool silent)
// REMOVED: static int Internal_Android_JNI_FileOpen(SDL_RWops* ctx)
// REMOVED: int Android_JNI_FileOpen(SDL_RWops* ctx, const char* fileName, const char* mode)
// REMOVED: size_t Android_JNI_FileRead(SDL_RWops* ctx, void* buffer, size_t size, size_t maxnum)
// REMOVED: size_t Android_JNI_FileWrite(SDL_RWops* ctx, const void* buffer, size_t size, size_t num)
// REMOVED: static int Internal_Android_JNI_FileClose(SDL_RWops* ctx, SDL_bool release)  (mostly called SDL_FreeRW(ctx))
// REMOVED: Sint64 Android_JNI_FileSize(SDL_RWops* ctx)
// REMOVED: Sint64 Android_JNI_FileSeek(SDL_RWops* ctx, Sint64 offset, int whence)
// REMOVED: int Android_JNI_FileClose(SDL_RWops* ctx)
// REMOVED: static jobject Android_JNI_GetSystemServiceObject(const char* name)
// REMOVED: int Android_JNI_SetClipboardText(const char* text)
// REMOVED: char* Android_JNI_GetClipboardText(void)
// REMOVED: SDL_bool Android_JNI_HasClipboardText(void)
// REMOVED: int Android_JNI_GetPowerInfo(int* plugged, int* charged, int* battery, int* seconds, int* percent)
// REMOVED: int Android_JNI_GetTouchDeviceIds(int **ids)
// REMOVED: void Android_JNI_PollInputDevices(void)
// REMOVED: int Android_JNI_SendMessage(int command, int param)
// REMOVED: void Android_JNI_SuspendScreenSaver(SDL_bool suspend)
// REMOVED: void Android_JNI_ShowTextInput(SDL_Rect *inputRect)
// REMOVED: void Android_JNI_HideTextInput(void)
// REMOVED: int Android_JNI_ShowMessageBox(const SDL_MessageBoxData *messageboxdata, int *buttonid)


//*********************
//*** END ORIG CODE ***
//*********************

int whichAudio = 1;
//******************************************************************************
//******************************************************************************
int Android_JNI_OpenAudioDevice(int iscapture, int sampleRate, int is16Bit, int channelCount, int desiredBufferFrames)
{
	int returnBuffers = 0;
	if (whichAudio == 0)	returnBuffers = Android_JNI_OpenAudioDevice_NEW(iscapture, sampleRate, is16Bit, channelCount, desiredBufferFrames);
	else					returnBuffers = Android_JNI_OpenAudioDevice_ORIG(iscapture, sampleRate, is16Bit, channelCount, desiredBufferFrames);

	return returnBuffers;
}

//******************************************************************************
//******************************************************************************
void * Android_JNI_GetAudioBuffer(void)
{
	void * returnBuffer = NULL;
	if (whichAudio == 0)	returnBuffer = Android_JNI_GetAudioBuffer_NEW();
	else					returnBuffer = Android_JNI_GetAudioBuffer_ORIG();

	return returnBuffer;
}

//******************************************************************************
//******************************************************************************
void Android_JNI_WriteAudioBuffer(void)
{
	if (whichAudio == 0)	Android_JNI_WriteAudioBuffer_NEW();
	else					Android_JNI_WriteAudioBuffer_ORIG();
}

//******************************************************************************
//******************************************************************************
void Android_JNI_CloseAudioDevice(const int iscapture)
{
	if (whichAudio == 0)	Android_JNI_CloseAudioDevice_NEW(iscapture);
	else					Android_JNI_CloseAudioDevice_ORIG(iscapture);
}

//******************************************************************************
//******************************************************************************
//******************************************************************************
// END Audio support
//******************************************************************************
//******************************************************************************
//******************************************************************************

//******************************************************************************
//******************************************************************************
void * SDL_AndroidGetJNIEnv()
{
	/* 2017.10.10 - RJS - JavaVM removal.
	return Android_JNI_GetEnv();
	*/
	return NULL;
}

//******************************************************************************
//******************************************************************************
void * SDL_AndroidGetActivity()
{
	// 2017.09.20 - RJS - removed meaningful functionality, wasn't being used, relied on Java
	return NULL;
}

//******************************************************************************
//******************************************************************************
extern const char * get_homedir_cstr();
const char * SDL_AndroidGetInternalStoragePath()
{
	static char *s_AndroidInternalFilesPath = NULL;

	if (!s_AndroidInternalFilesPath) {
		// RJS - 2017.08.10 - This uses Context and getFileDir on the Java side.  I'm going to 
		// hard code it here.  This should probably be passed from RA.  The above "if" statement
		// is weird given the NULL just above it.
		// RJS - 2017.08.11 - Update, while the hardcoded path is right, we can't rely on that 
		// containing anything.  So, likely the bmps, we need those downloaded by the user and placed
		// in the same directory for now.  Leaving hardcoding just to illustrate what is happening here.
		// Path is taken from the -homedir input, see the bmp loading for path.
		LOGI("daphne-libretro: In %s, internal path asked for, switching to external home path.  This path must contain needed file!!", __func__);
		static char strAndroidInternalFilesPath[100] = "";
		// static char strAndroidInternalFilesPath[100] = "/data/data/com.retroarch/files";
		// strcpy(strAndroidInternalFilesPath, "/storage/emulated/0/Roms/Daphne");
		strcpy(strAndroidInternalFilesPath, get_homedir_cstr());

		s_AndroidInternalFilesPath = strAndroidInternalFilesPath;
	}

	return s_AndroidInternalFilesPath;
}

//******************************************************************************
//******************************************************************************
int SDL_AndroidGetExternalStorageState()
{
	// 2017.09.21 - RJS - no support for external storage, support would need full rewrite anyways
	return 0;
}

//******************************************************************************
//******************************************************************************
const char * SDL_AndroidGetExternalStoragePath()
{
	// 2017.09.21 - RJS - no support for external storage, support would need full rewrite anyways
	return NULL;
}

#endif /* __ANDROID__ */

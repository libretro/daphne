#include <stdlib.h>

#include "SDL.h"

// RJS ADD START
#include "pch.h"
#include "main_android.h"
// RJS ADD END

#if defined(__ANDROID__)
#include "libretro/libretro_daphne.h"
/* 2017.10.10 - RJS - JavaVM removal.
extern JNIEnv *Android_JNI_GetEnv(void);
*/
extern void SDL_Android_Init(JNIEnv* env, jclass cls);
extern int Android_JNI_SetupThread(void);
#endif

//******************************************************************************
//******************************************************************************
void sdl_init()
{
	/* 2017.10.10 - RJS - JavaVM removal.
	JNIEnv *env = Android_JNI_GetEnv();
	if (env)
	{
		jobject instance = retro_get_nativeinstance();
		jclass clazz = (*env)->GetObjectClass(env, instance);
		SDL_Android_Init(env, clazz);
	}
	*/
	Android_JNI_SetupThread();
	SDL_Android_Init(NULL, NULL);
	SDL_SetMainReady();
}

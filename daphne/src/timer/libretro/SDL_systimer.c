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

#ifdef _WIN32

#ifdef _XBOX
#include <xtl.h>
#else
#include <windows.h>
#endif
#include <mmsystem.h>

#include "SDL_timer.h"

/* The first (low-resolution) ticks value of the application */
static DWORD start = 0;
static BOOL ticks_started = FALSE; 

/* Store if a high-resolution performance counter exists on the system */
static BOOL hires_timer_available;
/* The first high-resolution ticks value of the application */
static LARGE_INTEGER hires_start_ticks;
/* The number of ticks per second of the high-resolution performance counter */
static LARGE_INTEGER hires_ticks_per_second;

static void
SDL_SetSystemTimerResolution(const UINT uPeriod)
{
#ifndef __WINRT__
    static UINT timer_period = 0;

    if (uPeriod != timer_period) {
        if (timer_period) {
            timeEndPeriod(timer_period);
        }

        timer_period = uPeriod;

        if (timer_period) {
            timeBeginPeriod(timer_period);
        }
    }
#endif
}

static void
SDL_TimerResolutionChanged(void *userdata, const char *name, const char *oldValue, const char *hint)
{
    UINT uPeriod;

    /* Unless the hint says otherwise, let's have good sleep precision */
    if (hint && *hint) {
        uPeriod = atoi(hint);
    } else {
        uPeriod = 1;
    }
    if (uPeriod || oldValue != hint) {
        SDL_SetSystemTimerResolution(uPeriod);
    }
}

void
SDL_TicksInit(void)
{
    if (ticks_started) {
        return;
    }
    ticks_started = SDL_TRUE;

    /* Set first ticks value */
    /* QueryPerformanceCounter has had problems in the past, but lots of games
       use it, so we'll rely on it here.
     */
    if (QueryPerformanceFrequency(&hires_ticks_per_second) == TRUE) {
        hires_timer_available = TRUE;
        QueryPerformanceCounter(&hires_start_ticks);
    } else {
        hires_timer_available = FALSE;
#ifndef __WINRT__
        start = timeGetTime();
#endif /* __WINRT__ */
    }
}

void
SDL_TicksQuit(void)
{
    SDL_SetSystemTimerResolution(0);  /* always release our timer resolution request. */

    start = 0;
    ticks_started = SDL_FALSE;
}

uint32_t
SDL_GetTicks(void)
{
    DWORD now = 0;
    LARGE_INTEGER hires_now;

    if (!ticks_started) {
        SDL_TicksInit();
    }

    if (hires_timer_available) {
        QueryPerformanceCounter(&hires_now);

        hires_now.QuadPart -= hires_start_ticks.QuadPart;
        hires_now.QuadPart *= 1000;
        hires_now.QuadPart /= hires_ticks_per_second.QuadPart;

        return (DWORD) hires_now.QuadPart;
    } else {
#ifndef __WINRT__
        now = timeGetTime();
#endif /* __WINRT__ */
    }

    return (now - start);
}

void SDL_Delay(uint32_t ms)
{
    /* Sleep() is not publicly available to apps in early versions of WinRT.
     *
     * Visual C++ 2013 Update 4 re-introduced Sleep() for Windows 8.1 and
     * Windows Phone 8.1.
     *
     * Use the compiler version to determine availability.
     *
     * NOTE #1: _MSC_FULL_VER == 180030723 for Visual C++ 2013 Update 3.
     * NOTE #2: Visual C++ 2013, when compiling for Windows 8.0 and
     *    Windows Phone 8.0, uses the Visual C++ 2012 compiler to build
     *    apps and libraries.
     */
#if defined(__WINRT__) && defined(_MSC_FULL_VER) && (_MSC_FULL_VER <= 180030723)
    static HANDLE mutex = 0;
    if (!mutex)
        mutex = CreateEventEx(0, 0, 0, EVENT_ALL_ACCESS);
    WaitForSingleObjectEx(mutex, ms, FALSE);
#else
    if (!ticks_started)
        SDL_TicksInit();

    Sleep(ms);
#endif
}

/* vi: set ts=4 sw=4 expandtab: */
#else
#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#include "SDL_timer.h"
#include "assert.h"

#include "../../../main_android.h"

/* The clock_gettime provides monotonous time, so we should use it if
   it's available. The clock_gettime function is behind ifdef
   for __USE_POSIX199309
   Tommi Kyntola (tommi.kyntola@ray.fi) 27/09/2005
*/
/* Reworked monotonic clock to not assume the current system has one
   as not all linux kernels provide a monotonic clock (yeah recent ones
   probably do)
   Also added OS X Monotonic clock support
   Based on work in https://github.com/ThomasHabets/monotonic_clock
 */
#include <time.h>
#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

/* Use CLOCK_MONOTONIC_RAW, if available, which is not subject to adjustment by NTP */
#if HAVE_CLOCK_GETTIME
#ifdef CLOCK_MONOTONIC_RAW
#define SDL_MONOTONIC_CLOCK CLOCK_MONOTONIC_RAW
#else
#define SDL_MONOTONIC_CLOCK CLOCK_MONOTONIC
#endif
#endif

/* The first ticks value of the application */
#if HAVE_CLOCK_GETTIME
static struct timespec start_ts;
#elif defined(__APPLE__)
static uint64_t start_mach;
mach_timebase_info_data_t mach_base_info;
#endif
static SDL_bool has_monotonic_time = SDL_FALSE;
static struct timeval start_tv;
static SDL_bool ticks_started = SDL_FALSE;

void
SDL_TicksInit(void)
{
    if (ticks_started) {
        return;
    }
    ticks_started = SDL_TRUE;

    /* Set first ticks value */
#if HAVE_CLOCK_GETTIME
    if (clock_gettime(SDL_MONOTONIC_CLOCK, &start_ts) == 0) {
        has_monotonic_time = SDL_TRUE;
    } else
#elif defined(__APPLE__)
    kern_return_t ret = mach_timebase_info(&mach_base_info);
    if (ret == 0) {
        has_monotonic_time = SDL_TRUE;
        start_mach = mach_absolute_time();
    } else
#endif
    {
        gettimeofday(&start_tv, NULL);
    }
}

void
SDL_TicksQuit(void)
{
    ticks_started = SDL_FALSE;
}

uint32_t
SDL_GetTicks(void)
{
	uint32_t ticks = 0;
    if (!ticks_started)
        SDL_TicksInit();

    if (has_monotonic_time) {
#if HAVE_CLOCK_GETTIME
       struct timespec now;
       clock_gettime(SDL_MONOTONIC_CLOCK, &now);
       ticks = (now.tv_sec - start_ts.tv_sec) * 1000 + (now.tv_nsec -
             start_ts.tv_nsec) / 1000000;
#elif defined(__APPLE__)
       uint64_t now = mach_absolute_time();
       ticks = (uint32_t)((((now - start_mach) * mach_base_info.numer) / mach_base_info.denom) / 1000000);
#else
       assert(SDL_FALSE);
       ticks = 0;
#endif
    } else {
        struct timeval now;

		gettimeofday(&now, NULL);
		ticks = (uint32_t)((now.tv_sec - start_tv.tv_sec) * 1000 + (now.tv_usec - start_tv.tv_usec) / 1000);
	}
	return (ticks);
}

void
SDL_Delay(uint32_t ms)
{
	int was_error = 0;
	struct timespec elapsed, tv;

	elapsed.tv_sec = ms / 1000;
	elapsed.tv_nsec = (ms % 1000) * 1000000;

	do
	{
		errno = 0;

		tv.tv_sec	= elapsed.tv_sec;
		tv.tv_nsec	= elapsed.tv_nsec;

		was_error	= nanosleep(&tv, &elapsed);
	} while (was_error && (errno == EINTR));
}

#endif

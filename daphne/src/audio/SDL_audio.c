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

/* Allow access to a raw mixing buffer */

#include "SDL.h"
#include "SDL_audio.h"
#include "SDL_audio_c.h"
#include "SDL_sysaudio.h"
#include "../thread/SDL_systhread.h"

#define _THIS SDL_AudioDevice *_this

static SDL_AudioDriver current_audio;
static SDL_AudioDevice *open_devices[16];

/*
 * Not all of these will be compiled and linked in, but it's convenient
 *  to have a complete list here and saves yet-another block of #ifdefs...
 *  Please see bootstrap[], below, for the actual #ifdef mess.
 */
extern AudioBootStrap PULSEAUDIO_bootstrap;
extern AudioBootStrap ALSA_bootstrap;
extern AudioBootStrap SNDIO_bootstrap;
extern AudioBootStrap BSD_AUDIO_bootstrap;
extern AudioBootStrap DSP_bootstrap;
extern AudioBootStrap QSAAUDIO_bootstrap;
extern AudioBootStrap SUNAUDIO_bootstrap;
extern AudioBootStrap ARTS_bootstrap;
extern AudioBootStrap ESD_bootstrap;
extern AudioBootStrap NACLAUDIO_bootstrap;
extern AudioBootStrap NAS_bootstrap;
extern AudioBootStrap PAUDIO_bootstrap;
extern AudioBootStrap HAIKUAUDIO_bootstrap;
extern AudioBootStrap COREAUDIO_bootstrap;
extern AudioBootStrap DUMMYAUDIO_bootstrap;
extern AudioBootStrap FUSIONSOUND_bootstrap;
extern AudioBootStrap ANDROIDAUDIO_bootstrap;
extern AudioBootStrap PSPAUDIO_bootstrap;
extern AudioBootStrap SNDIO_bootstrap;
extern AudioBootStrap EMSCRIPTENAUDIO_bootstrap;

/* Available audio drivers */
static const AudioBootStrap *const bootstrap[] = {
#if SDL_AUDIO_DRIVER_PULSEAUDIO
    &PULSEAUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_ALSA
    &ALSA_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_SNDIO
    &SNDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_BSD
    &BSD_AUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_OSS
    &DSP_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_QSA
    &QSAAUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_SUNAUDIO
    &SUNAUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_ARTS
    &ARTS_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_ESD
    &ESD_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_NACL
    &NACLAUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_NAS
    &NAS_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_PAUDIO
    &PAUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_HAIKU
    &HAIKUAUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_COREAUDIO
    &COREAUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_DUMMY
    &DUMMYAUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_FUSIONSOUND
    &FUSIONSOUND_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_ANDROID
    &ANDROIDAUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_PSP
    &PSPAUDIO_bootstrap,
#endif
#if SDL_AUDIO_DRIVER_EMSCRIPTEN
    &EMSCRIPTENAUDIO_bootstrap,
#endif
    NULL
};

/* buffer queueing support... */

/* this expects that you managed thread safety elsewhere. */
static void
free_audio_queue(SDL_AudioBufferQueue *packet)
{
    while (packet) {
        SDL_AudioBufferQueue *next = packet->next;
        free(packet);
        packet = next;
    }
}

/* NOTE: This assumes you'll hold the mixer lock before calling! */
static int
queue_audio_to_device(SDL_AudioDevice *device, const Uint8 *data, Uint32 len)
{
    SDL_AudioBufferQueue *orighead;
    SDL_AudioBufferQueue *origtail;
    Uint32 origlen;
    Uint32 datalen;

    orighead = device->buffer_queue_head;
    origtail = device->buffer_queue_tail;
    origlen = origtail ? origtail->datalen : 0;

    while (len > 0) {
        SDL_AudioBufferQueue *packet = device->buffer_queue_tail;
        SDL_assert(!packet || (packet->datalen <= SDL_AUDIOBUFFERQUEUE_PACKETLEN));
        if (!packet || (packet->datalen >= SDL_AUDIOBUFFERQUEUE_PACKETLEN)) {
            /* tail packet missing or completely full; we need a new packet. */
            packet = device->buffer_queue_pool;
            if (packet != NULL) {
                /* we have one available in the pool. */
                device->buffer_queue_pool = packet->next;
            } else {
                /* Have to allocate a new one! */
                packet = (SDL_AudioBufferQueue *)malloc(sizeof (SDL_AudioBufferQueue));
                if (packet == NULL) {
                    /* uhoh, reset so we've queued nothing new, free what we can. */
                    if (!origtail) {
                        packet = device->buffer_queue_head;  /* whole queue. */
                    } else {
                        packet = origtail->next;  /* what we added to existing queue. */
                        origtail->next = NULL;
                        origtail->datalen = origlen;
                    }
                    device->buffer_queue_head = orighead;
                    device->buffer_queue_tail = origtail;
                    device->buffer_queue_pool = NULL;

                    free_audio_queue(packet);  /* give back what we can. */

                    return SDL_OutOfMemory();
                }
            }
            packet->datalen = 0;
            packet->startpos = 0;
            packet->next = NULL;

            SDL_assert((device->buffer_queue_head != NULL) == (device->queued_bytes != 0));
            if (device->buffer_queue_tail == NULL) {
                device->buffer_queue_head = packet;
            } else {
                device->buffer_queue_tail->next = packet;
            }
            device->buffer_queue_tail = packet;
        }

        datalen = SDL_min(len, SDL_AUDIOBUFFERQUEUE_PACKETLEN - packet->datalen);
        SDL_memcpy(packet->data + packet->datalen, data, datalen);
        data += datalen;
        len -= datalen;
        packet->datalen += datalen;
        device->queued_bytes += datalen;
    }

    return 0;
}

/* NOTE: This assumes you'll hold the mixer lock before calling! */
static Uint32
dequeue_audio_from_device(SDL_AudioDevice *device, Uint8 *stream, Uint32 len)
{
    SDL_AudioBufferQueue *packet;
    Uint8 *ptr = stream;

    while ((len > 0) && ((packet = device->buffer_queue_head) != NULL)) {
        const Uint32 avail = packet->datalen - packet->startpos;
        const Uint32 cpy = SDL_min(len, avail);
        SDL_assert(device->queued_bytes >= avail);

        SDL_memcpy(ptr, packet->data + packet->startpos, cpy);
        packet->startpos += cpy;
        ptr += cpy;
        device->queued_bytes -= cpy;
        len -= cpy;

        if (packet->startpos == packet->datalen) {  /* packet is done, put it in the pool. */
            device->buffer_queue_head = packet->next;
            SDL_assert((packet->next != NULL) || (packet == device->buffer_queue_tail));
            packet->next = device->buffer_queue_pool;
            device->buffer_queue_pool = packet;
        }
    }

    SDL_assert((device->buffer_queue_head != NULL) == (device->queued_bytes != 0));

    if (device->buffer_queue_head == NULL) {
        device->buffer_queue_tail = NULL;  /* in case we drained the queue entirely. */
    }

    return (Uint32) (ptr - stream);
}

static void SDLCALL
SDL_BufferQueueDrainCallback(void *userdata, Uint8 *stream, int len)
{
    /* this function always holds the mixer lock before being called. */
    SDL_AudioDevice *device = (SDL_AudioDevice *) userdata;
    Uint32 written;

    SDL_assert(device != NULL);  /* this shouldn't ever happen, right?! */
    SDL_assert(!device->iscapture);  /* this shouldn't ever happen, right?! */
    SDL_assert(len >= 0);  /* this shouldn't ever happen, right?! */

    written = dequeue_audio_from_device(device, stream, (Uint32) len);
    stream += written;
    len -= (int) written;

    if (len > 0) {  /* fill any remaining space in the stream with silence. */
        SDL_assert(device->buffer_queue_head == NULL);
        SDL_memset(stream, device->spec.silence, len);
    }
}

static void SDLCALL
SDL_BufferQueueFillCallback(void *userdata, Uint8 *stream, int len)
{
    /* this function always holds the mixer lock before being called. */
    SDL_AudioDevice *device = (SDL_AudioDevice *) userdata;

    SDL_assert(device != NULL);  /* this shouldn't ever happen, right?! */
    SDL_assert(device->iscapture);  /* this shouldn't ever happen, right?! */
    SDL_assert(len >= 0);  /* this shouldn't ever happen, right?! */

    /* note that if this needs to allocate more space and run out of memory,
       we have no choice but to quietly drop the data and hope it works out
       later, but you probably have bigger problems in this case anyhow. */
    queue_audio_to_device(device, stream, (Uint32) len);
}

// RJS HERE - SDL run audio loop.
/* The general mixing thread function */
static int SDLCALL
SDL_RunAudio(void *devicep)
{
    SDL_AudioDevice *device = (SDL_AudioDevice *) devicep;
    const int silence = (int) device->spec.silence;
    const Uint32 delay = ((device->spec.samples * 1000) / device->spec.freq);
    const int stream_len = (device->convert.needed) ? device->convert.len : device->spec.size;
    Uint8 *stream;
    void *udata = device->spec.userdata;
    void (SDLCALL *callback) (void *, Uint8 *, int) = device->spec.callback;

    SDL_assert(!device->iscapture);

    /* The audio mixing is always a high priority thread */
    SDL_SetThreadPriority(SDL_THREAD_PRIORITY_HIGH);

    /* Perform any thread setup */
    device->threadid = SDL_ThreadID();
    current_audio.impl.ThreadInit(device);

    /* Loop, filling the audio buffers */
    while (!SDL_AtomicGet(&device->shutdown)) {
		/* Fill the current buffer with sound */
        if (device->convert.needed) {
            stream = device->convert.buf;
        } else if (SDL_AtomicGet(&device->enabled)) {
			// Calls: Android_JNI_GetAudioBuffer
			stream = current_audio.impl.GetDeviceBuf(device);
		} else {
            /* if the device isn't enabled, we still write to the
               fake_stream, so the app's callback will fire with
               a regular frequency, in case they depend on that
               for timing or progress. They can use hotplug
               now to know if the device failed. */
            stream = NULL;
        }

		if (stream == NULL) {
            stream = device->fake_stream;
        }

		// LOGI("In SDL_RunAudio, in while, if 2.");
		/* !!! FIXME: this should be LockDevice. */
        if ( SDL_AtomicGet(&device->enabled) ) {
            SDL_LockMutex(device->mixer_lock);
            if (SDL_AtomicGet(&device->paused)) {
				SDL_memset(stream, silence, stream_len);
            } else {
				// SDL_BufferQueueDrainCallback
                (*callback) (udata, stream, stream_len);
            }
            SDL_UnlockMutex(device->mixer_lock);
        }

		/* Convert the audio if necessary */
        if (device->convert.needed && SDL_AtomicGet(&device->enabled)) {
			SDL_ConvertAudio(&device->convert);
            stream = current_audio.impl.GetDeviceBuf(device);
            if (stream == NULL) {
                stream = device->fake_stream;
            } else {
                SDL_memcpy(stream, device->convert.buf,
                           device->convert.len_cvt);
            }
        }

		/* Ready current buffer for play and change current buffer */
        if (stream == device->fake_stream) {
			SDL_Delay(delay);
        } else {
			// 2017.08.17 - RJS - PlayDevice eventually sets the audio buffer flag as ready.
			// Set here: impl->PlayDevice = ANDROIDAUDIO_PlayDevice;
			// Which does this: ANDROIDAUDIO_PlayDevice calls Android_JNI_WriteAudioBuffer();
			// Which does this: Android_JNI_WriteAudioBuffer calls cb_audiosamplebatch(audioBufferToUse, audioBufferSize / 2);
			// When core is paused, the audio thread pauses inside PlayDevice == ANDROIDAUDIO_PlayDevice == Android_JNI_WriteAudioBuffer.
			current_audio.impl.PlayDevice(device);
			// WaitDevice on Android is a stub (nop).
			current_audio.impl.WaitDevice(device);
        }
	}

	current_audio.impl.PrepareToClose(device);

    /* Wait for the audio to drain. */
    SDL_Delay(((device->spec.samples * 1000) / device->spec.freq) * 2);


    return 0;
}

static SDL_AudioFormat
SDL_ParseAudioFormat(const char *string)
{
#define CHECK_FMT_STRING(x) if (SDL_strcmp(string, #x) == 0) return AUDIO_##x
    CHECK_FMT_STRING(U8);
    CHECK_FMT_STRING(S8);
    CHECK_FMT_STRING(U16LSB);
    CHECK_FMT_STRING(S16LSB);
    CHECK_FMT_STRING(U16MSB);
    CHECK_FMT_STRING(S16MSB);
    CHECK_FMT_STRING(U16SYS);
    CHECK_FMT_STRING(S16SYS);
    CHECK_FMT_STRING(U16);
    CHECK_FMT_STRING(S16);
    CHECK_FMT_STRING(S32LSB);
    CHECK_FMT_STRING(S32MSB);
    CHECK_FMT_STRING(S32SYS);
    CHECK_FMT_STRING(S32);
    CHECK_FMT_STRING(F32LSB);
    CHECK_FMT_STRING(F32MSB);
    CHECK_FMT_STRING(F32SYS);
    CHECK_FMT_STRING(F32);
#undef CHECK_FMT_STRING
    return 0;
}

int
SDL_GetNumAudioDrivers(void)
{
    return SDL_arraysize(bootstrap) - 1;
}

const char *
SDL_GetAudioDriver(int index)
{
    if (index >= 0 && index < SDL_GetNumAudioDrivers()) {
        return bootstrap[index]->name;
    }
    return NULL;
}

int
SDL_AudioInit(const char *driver_name)
{
    int i = 0;
    int initialized = 0;
    int tried_to_init = 0;

    SDL_zero(current_audio);
    SDL_zero(open_devices);

    /* Select the proper audio driver */
    if (driver_name == NULL) {
        driver_name = SDL_getenv("SDL_AUDIODRIVER");
    }

    for (i = 0; (!initialized) && (bootstrap[i]); ++i) {
        /* make sure we should even try this driver before doing so... */
        const AudioBootStrap *backend = bootstrap[i];
        if ((driver_name && (SDL_strncasecmp(backend->name, driver_name, SDL_strlen(driver_name)) != 0)) ||
            (!driver_name && backend->demand_only)) {
            continue;
        }

        tried_to_init = 1;
        SDL_zero(current_audio);
        current_audio.name = backend->name;
        current_audio.desc = backend->desc;
        initialized = backend->init(&current_audio.impl);
    }

    if (!initialized) {
        /* specific drivers will set the error message if they fail... */
        if (!tried_to_init) {
            if (driver_name) {
                SDL_SetError("Audio target '%s' not available", driver_name);
            } else {
                SDL_SetError("No available audio device");
            }
        }

        SDL_zero(current_audio);
        return -1;            /* No driver was available, so fail. */
    }

    current_audio.detectionLock = SDL_CreateMutex();

    /* Make sure we have a list of devices available at startup. */
    current_audio.impl.DetectDevices();

    return 0;
}

/*
 * Get the current audio driver name
 */
const char *
SDL_GetCurrentAudioDriver()
{
    return current_audio.name;
}

/* Clean out devices that we've removed but had to keep around for stability. */
static void
clean_out_device_list(SDL_AudioDeviceItem **devices, int *devCount, SDL_bool *removedFlag)
{
    SDL_AudioDeviceItem *item = *devices;
    SDL_AudioDeviceItem *prev = NULL;
    int total = 0;

    while (item) {
        SDL_AudioDeviceItem *next = item->next;
        if (item->handle != NULL) {
            total++;
            prev = item;
        } else {
            if (prev) {
                prev->next = next;
            } else {
                *devices = next;
            }
            free(item);
        }
        item = next;
    }

    *devCount = total;
    *removedFlag = SDL_FALSE;
}


int
SDL_GetNumAudioDevices(int iscapture)
{
    int retval = 0;

    SDL_LockMutex(current_audio.detectionLock);
    if (current_audio.outputDevicesRemoved) {
        clean_out_device_list(&current_audio.outputDevices, &current_audio.outputDeviceCount, &current_audio.outputDevicesRemoved);
        current_audio.outputDevicesRemoved = SDL_FALSE;
    }

    retval = current_audio.outputDeviceCount;
    SDL_UnlockMutex(current_audio.detectionLock);

    return retval;
}


const char *
SDL_GetAudioDeviceName(int index, int iscapture)
{
    const char *retval = NULL;

    if ((iscapture) && (!current_audio.impl.HasCaptureSupport)) {
        SDL_SetError("No capture support");
        return NULL;
    }

    if (index >= 0) {
        SDL_AudioDeviceItem *item;
        int i;

        SDL_LockMutex(current_audio.detectionLock);
        item = iscapture ? current_audio.inputDevices : current_audio.outputDevices;
        i = iscapture ? current_audio.inputDeviceCount : current_audio.outputDeviceCount;
        if (index < i) {
            for (i--; i > index; i--, item = item->next) {
                SDL_assert(item != NULL);
            }
            SDL_assert(item != NULL);
            retval = item->name;
        }
        SDL_UnlockMutex(current_audio.detectionLock);
    }

    if (retval == NULL) {
        SDL_SetError("No such device");
    }

    return retval;
}


static void
close_audio_device(SDL_AudioDevice * device)
{
	if (!device) {
        return;
    }

	if (device->id > 0) {
        SDL_AudioDevice *opendev = open_devices[device->id - 1];
        SDL_assert((opendev == device) || (opendev == NULL));
        if (opendev == device) {
            open_devices[device->id - 1] = NULL;
        }
    }

	SDL_AtomicSet(&device->shutdown, 1);
	SDL_AtomicSet(&device->enabled, 0);
	if (device->thread != NULL) {
		// RJS HERE COMPLETE HACK
		// extern void Android_JNI_WriteAudioBuffer();
		// Android_JNI_WriteAudioBuffer();
        SDL_WaitThread(device->thread, NULL);
    }
	if (device->mixer_lock != NULL) {
        SDL_DestroyMutex(device->mixer_lock);
    }
	free(device->fake_stream);
	if (device->convert.needed) {
        free(device->convert.buf);
    }
	if (device->hidden != NULL) {
        current_audio.impl.CloseDevice(device);
    }

	free_audio_queue(device->buffer_queue_head);
    free_audio_queue(device->buffer_queue_pool);

	free(device);

}


/*
 * Sanity check desired AudioSpec for SDL_OpenAudio() in (orig).
 *  Fills in a sanitized copy in (prepared).
 *  Returns non-zero if okay, zero on fatal parameters in (orig).
 */
static int
prepare_audiospec(const SDL_AudioSpec * orig, SDL_AudioSpec * prepared)
{
    SDL_memcpy(prepared, orig, sizeof(SDL_AudioSpec));

    if (orig->freq == 0) {
        const char *env = SDL_getenv("SDL_AUDIO_FREQUENCY");
        if ((!env) || ((prepared->freq = SDL_atoi(env)) == 0)) {
            prepared->freq = 22050;     /* a reasonable default */
        }
    }

    if (orig->format == 0) {
        const char *env = SDL_getenv("SDL_AUDIO_FORMAT");
        if ((!env) || ((prepared->format = SDL_ParseAudioFormat(env)) == 0)) {
            prepared->format = AUDIO_S16;       /* a reasonable default */
        }
    }

    switch (orig->channels) {
    case 0:{
            const char *env = SDL_getenv("SDL_AUDIO_CHANNELS");
            if ((!env) || ((prepared->channels = (Uint8) SDL_atoi(env)) == 0)) {
                prepared->channels = 2; /* a reasonable default */
            }
            break;
        }
    case 1:                    /* Mono */
    case 2:                    /* Stereo */
    case 4:                    /* surround */
    case 6:                    /* surround with center and lfe */
        break;
    default:
        SDL_SetError("Unsupported number of audio channels.");
        return 0;
    }

    if (orig->samples == 0) {
        const char *env = SDL_getenv("SDL_AUDIO_SAMPLES");
        if ((!env) || ((prepared->samples = (Uint16) SDL_atoi(env)) == 0)) {
            /* Pick a default of ~46 ms at desired frequency */
            /* !!! FIXME: remove this when the non-Po2 resampling is in. */
            const int samples = (prepared->freq / 1000) * 46;
            int power2 = 1;
            while (power2 < samples) {
                power2 *= 2;
            }
            prepared->samples = power2;
        }
    }

    /* Calculate the silence and size of the audio specification */
    SDL_CalculateAudioSpec(prepared);

    return 1;
}

static SDL_AudioDeviceID
open_audio_device(const char *devname, int iscapture,
                  const SDL_AudioSpec * desired, SDL_AudioSpec * obtained,
                  int allowed_changes, int min_id)
{
    const SDL_bool is_internal_thread = (desired->callback != NULL);
    SDL_AudioDeviceID id = 0;
    SDL_AudioSpec _obtained;
    SDL_AudioDevice *device;
    SDL_bool build_cvt;
    void *handle = NULL;
    int i = 0;

    if ((iscapture) && (!current_audio.impl.HasCaptureSupport)) {
        SDL_SetError("No capture support");
        return 0;
    }

    /* !!! FIXME: there is a race condition here if two devices open from two threads at once. */
    /* Find an available device ID... */
    for (id = min_id - 1; id < SDL_arraysize(open_devices); id++) {
        if (open_devices[id] == NULL) {
            break;
        }
    }

    if (id == SDL_arraysize(open_devices)) {
        SDL_SetError("Too many open audio devices");
        return 0;
    }

    if (!obtained) {
        obtained = &_obtained;
    }
    if (!prepare_audiospec(desired, obtained)) {
        return 0;
    }

    /* If app doesn't care about a specific device, let the user override. */
    if (devname == NULL) {
        devname = SDL_getenv("SDL_AUDIO_DEVICE_NAME");
    }

    /*
     * Catch device names at the high level for the simple case...
     * This lets us have a basic "device enumeration" for systems that
     *  don't have multiple devices, but makes sure the device name is
     *  always NULL when it hits the low level.
     *
     * Also make sure that the simple case prevents multiple simultaneous
     *  opens of the default system device.
     */

    if ((iscapture) && (current_audio.impl.OnlyHasDefaultCaptureDevice)) {
        if ((devname) && (SDL_strcmp(devname, DEFAULT_INPUT_DEVNAME) != 0)) {
            SDL_SetError("No such device");
            return 0;
        }
        devname = NULL;

        for (i = 0; i < SDL_arraysize(open_devices); i++) {
            if ((open_devices[i]) && (open_devices[i]->iscapture)) {
                SDL_SetError("Audio device already open");
                return 0;
            }
        }
    } else if ((!iscapture) && (current_audio.impl.OnlyHasDefaultOutputDevice)) {
        if ((devname) && (SDL_strcmp(devname, DEFAULT_OUTPUT_DEVNAME) != 0)) {
            SDL_SetError("No such device");
            return 0;
        }
        devname = NULL;

        for (i = 0; i < SDL_arraysize(open_devices); i++) {
            if ((open_devices[i]) && (!open_devices[i]->iscapture)) {
                SDL_SetError("Audio device already open");
                return 0;
            }
        }
    } else if (devname != NULL) {
        /* if the app specifies an exact string, we can pass the backend
           an actual device handle thingey, which saves them the effort of
           figuring out what device this was (such as, reenumerating
           everything again to find the matching human-readable name).
           It might still need to open a device based on the string for,
           say, a network audio server, but this optimizes some cases. */
        SDL_AudioDeviceItem *item;
        SDL_LockMutex(current_audio.detectionLock);
        for (item = iscapture ? current_audio.inputDevices : current_audio.outputDevices; item; item = item->next) {
            if ((item->handle != NULL) && (SDL_strcmp(item->name, devname) == 0)) {
                handle = item->handle;
                break;
            }
        }
        SDL_UnlockMutex(current_audio.detectionLock);
    }

    if (!current_audio.impl.AllowsArbitraryDeviceNames) {
        /* has to be in our device list, or the default device. */
        if ((handle == NULL) && (devname != NULL)) {
            SDL_SetError("No such device.");
            return 0;
        }
    }

    device = (SDL_AudioDevice *)calloc(1, sizeof (SDL_AudioDevice));
    if (device == NULL) {
        SDL_OutOfMemory();
        return 0;
    }
    device->id = id + 1;
    device->spec = *obtained;
    device->iscapture = iscapture ? SDL_TRUE : SDL_FALSE;
    device->handle = handle;

    SDL_AtomicSet(&device->shutdown, 0);  /* just in case. */
    SDL_AtomicSet(&device->paused, 1);
    SDL_AtomicSet(&device->enabled, 1);

    /* Create a mutex for locking the sound buffers */
    if (!current_audio.impl.SkipMixerLock) {
        device->mixer_lock = SDL_CreateMutex();
        if (device->mixer_lock == NULL) {
            close_audio_device(device);
            SDL_SetError("Couldn't create mixer lock");
            return 0;
        }
    }

	if (current_audio.impl.OpenDevice(device, handle, devname, iscapture) < 0) {
        close_audio_device(device);
        return 0;
    }

    /* if your target really doesn't need it, set it to 0x1 or something. */
    /* otherwise, close_audio_device() won't call impl.CloseDevice(). */
    SDL_assert(device->hidden != NULL);

    /* See if we need to do any conversion */
    build_cvt = SDL_FALSE;
    if (obtained->freq != device->spec.freq) {
        if (allowed_changes & SDL_AUDIO_ALLOW_FREQUENCY_CHANGE) {
            obtained->freq = device->spec.freq;
        } else {
            build_cvt = SDL_TRUE;
        }
    }
    if (obtained->format != device->spec.format) {
        if (allowed_changes & SDL_AUDIO_ALLOW_FORMAT_CHANGE) {
            obtained->format = device->spec.format;
        } else {
            build_cvt = SDL_TRUE;
        }
    }
    if (obtained->channels != device->spec.channels) {
        if (allowed_changes & SDL_AUDIO_ALLOW_CHANNELS_CHANGE) {
            obtained->channels = device->spec.channels;
        } else {
            build_cvt = SDL_TRUE;
        }
    }

    /* If the audio driver changes the buffer size, accept it.
       This needs to be done after the format is modified above,
       otherwise it might not have the correct buffer size.
     */
    if (device->spec.samples != obtained->samples) {
        obtained->samples = device->spec.samples;
        SDL_CalculateAudioSpec(obtained);
    }

    if (build_cvt) {
        /* Build an audio conversion block */
        if (SDL_BuildAudioCVT(&device->convert,
                              obtained->format, obtained->channels,
                              obtained->freq,
                              device->spec.format, device->spec.channels,
                              device->spec.freq) < 0) {
            close_audio_device(device);
            return 0;
        }
        if (device->convert.needed) {
            device->convert.len = (int) (((double) device->spec.size) /
                                         device->convert.len_ratio);

            device->convert.buf =
                (Uint8 *)malloc(device->convert.len *
                                            device->convert.len_mult);
            if (device->convert.buf == NULL) {
                close_audio_device(device);
                SDL_OutOfMemory();
                return 0;
            }
        }
    }

    if (device->spec.callback == NULL) {  /* use buffer queueing? */
        /* pool a few packets to start. Enough for two callbacks. */
        const int packetlen = SDL_AUDIOBUFFERQUEUE_PACKETLEN;
        const int wantbytes = ((device->convert.needed) ? device->convert.len : device->spec.size) * 2;
        const int wantpackets = (wantbytes / packetlen) + ((wantbytes % packetlen) ? packetlen : 0);
        for (i = 0; i < wantpackets; i++) {
            SDL_AudioBufferQueue *packet = (SDL_AudioBufferQueue *)malloc(sizeof (SDL_AudioBufferQueue));
            if (packet) { /* don't care if this fails, we'll deal later. */
                packet->datalen = 0;
                packet->startpos = 0;
                packet->next = device->buffer_queue_pool;
                device->buffer_queue_pool = packet;
            }
        }

        device->spec.callback = iscapture ? SDL_BufferQueueFillCallback : SDL_BufferQueueDrainCallback;
        device->spec.userdata = device;
    }

    /* add it to our list of open devices. */
    open_devices[id] = device;

    /* Start the audio thread if necessary */
    if (!current_audio.impl.ProvidesOwnCallbackThread) {
        /* Start the audio thread */
        /* !!! FIXME: we don't force the audio thread stack size here if it calls into user code, but maybe we should? */
        /* buffer queueing callback only needs a few bytes, so make the stack tiny. */
        const size_t stacksize = is_internal_thread ? 64 * 1024 : 0;
        char threadname[64];

        /* Allocate a fake audio buffer; only used by our internal threads. */
        Uint32 stream_len = (device->convert.needed) ? device->convert.len_cvt : 0;
        if (device->spec.size > stream_len) {
            stream_len = device->spec.size;
        }
        SDL_assert(stream_len > 0);

        device->fake_stream = (Uint8 *)malloc(stream_len);
        if (device->fake_stream == NULL) {
            close_audio_device(device);
            SDL_OutOfMemory();
            return 0;
        }

        SDL_snprintf(threadname, sizeof (threadname), "SDLAudioDev%d", (int) device->id);
		// device->thread = SDL_CreateThreadInternal(SDL_RunAudio, threadname, stacksize, device);
		device->thread = SDL_CreateThreadInternal(SDL_RunAudio, threadname, stacksize, device);

        if (device->thread == NULL) {
            close_audio_device(device);
            SDL_SetError("Couldn't create audio thread");
            return 0;
        }
    }

    return device->id;
}


int
SDL_OpenAudio(SDL_AudioSpec * desired, SDL_AudioSpec * obtained)
{
    SDL_AudioDeviceID id = 0;

    /* SDL_OpenAudio() is legacy and can only act on Device ID #1. */
    if (open_devices[0] != NULL) {
        SDL_SetError("Audio device is already opened");
        return -1;
    }

	if (obtained) {
        id = open_audio_device(NULL, 0, desired, obtained,
                               SDL_AUDIO_ALLOW_ANY_CHANGE, 1);
    } else {
        id = open_audio_device(NULL, 0, desired, NULL, 0, 1);
    }

    SDL_assert((id == 0) || (id == 1));
    return (id == 0) ? -1 : 0;
}

SDL_AudioDeviceID
SDL_OpenAudioDevice(const char *device, int iscapture,
                    const SDL_AudioSpec * desired, SDL_AudioSpec * obtained,
                    int allowed_changes)
{
    return open_audio_device(device, iscapture, desired, obtained,
                             allowed_changes, 2);
}

void
SDL_CloseAudioDevice(SDL_AudioDeviceID devid)
{
}

void
SDL_CloseAudio(void)
{
    SDL_CloseAudioDevice(1);
}

void
SDL_AudioQuit(void)
{
    SDL_AudioDeviceID i;

    if (!current_audio.name) {  /* not initialized?! */
        return;
    }

    for (i = 0; i < SDL_arraysize(open_devices); i++) {
        close_audio_device(open_devices[i]);
    }

    /* Free the driver data */
    current_audio.impl.Deinitialize();

    SDL_DestroyMutex(current_audio.detectionLock);

    SDL_zero(current_audio);
    SDL_zero(open_devices);
}

#define NUM_FORMATS 10
static int format_idx;
static int format_idx_sub;
static SDL_AudioFormat format_list[NUM_FORMATS][NUM_FORMATS] = {
    {AUDIO_U8, AUDIO_S8, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB,
     AUDIO_U16MSB, AUDIO_S32LSB, AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB},
    {AUDIO_S8, AUDIO_U8, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB,
     AUDIO_U16MSB, AUDIO_S32LSB, AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB},
    {AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_S32LSB,
     AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_S16MSB, AUDIO_S16LSB, AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_S32MSB,
     AUDIO_S32LSB, AUDIO_F32MSB, AUDIO_F32LSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_S16LSB, AUDIO_S16MSB, AUDIO_S32LSB,
     AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_S16MSB, AUDIO_S16LSB, AUDIO_S32MSB,
     AUDIO_S32LSB, AUDIO_F32MSB, AUDIO_F32LSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_S32LSB, AUDIO_S32MSB, AUDIO_F32LSB, AUDIO_F32MSB, AUDIO_S16LSB,
     AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_S32MSB, AUDIO_S32LSB, AUDIO_F32MSB, AUDIO_F32LSB, AUDIO_S16MSB,
     AUDIO_S16LSB, AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_F32LSB, AUDIO_F32MSB, AUDIO_S32LSB, AUDIO_S32MSB, AUDIO_S16LSB,
     AUDIO_S16MSB, AUDIO_U16LSB, AUDIO_U16MSB, AUDIO_U8, AUDIO_S8},
    {AUDIO_F32MSB, AUDIO_F32LSB, AUDIO_S32MSB, AUDIO_S32LSB, AUDIO_S16MSB,
     AUDIO_S16LSB, AUDIO_U16MSB, AUDIO_U16LSB, AUDIO_U8, AUDIO_S8},
};

SDL_AudioFormat
SDL_FirstAudioFormat(SDL_AudioFormat format)
{
    for (format_idx = 0; format_idx < NUM_FORMATS; ++format_idx) {
        if (format_list[format_idx][0] == format) {
            break;
        }
    }
    format_idx_sub = 0;
    return SDL_NextAudioFormat();
}

SDL_AudioFormat
SDL_NextAudioFormat(void)
{
    if ((format_idx == NUM_FORMATS) || (format_idx_sub == NUM_FORMATS)) {
        return 0;
    }
    return format_list[format_idx][format_idx_sub++];
}

void
SDL_CalculateAudioSpec(SDL_AudioSpec * spec)
{
    switch (spec->format) {
    case AUDIO_U8:
        spec->silence = 0x80;
        break;
    default:
        spec->silence = 0x00;
        break;
    }
    spec->size = SDL_AUDIO_BITSIZE(spec->format) / 8;
    spec->size *= spec->channels;
    spec->size *= spec->samples;
}

/* vi: set ts=4 sw=4 expandtab: */

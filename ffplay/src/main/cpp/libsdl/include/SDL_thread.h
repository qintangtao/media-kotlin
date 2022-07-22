#ifndef SDL_thread_h_
#define SDL_thread_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "SDL_config.h"

/* The SDL thread structure, defined in SDL_thread.c */
struct SDL_Thread;
typedef struct SDL_Thread SDL_Thread;

/* The SDL thread ID */
typedef unsigned long SDL_threadID;

/* Thread local storage ID, 0 is the invalid ID */
typedef unsigned int SDL_TLSID;

typedef enum {
    SDL_THREAD_PRIORITY_LOW,
    SDL_THREAD_PRIORITY_NORMAL,
    SDL_THREAD_PRIORITY_HIGH,
    SDL_THREAD_PRIORITY_TIME_CRITICAL
} SDL_ThreadPriority;

typedef int (SDLCALL* SDL_ThreadFunction) (void *data);

extern DECLSPEC SDL_Thread *SDLCALL
SDL_CreateThread(SDL_ThreadFunction fn, const char *name, void *data);

extern DECLSPEC const char *SDLCALL SDL_GetThreadName(SDL_Thread *thread);

extern DECLSPEC SDL_threadID SDLCALL SDL_ThreadID(void);

extern DECLSPEC SDL_threadID SDLCALL SDL_GetThreadID(SDL_Thread * thread);

extern DECLSPEC void SDLCALL SDL_WaitThread(SDL_Thread * thread, int *status);

extern DECLSPEC void SDLCALL SDL_DetachThread(SDL_Thread * thread);

extern DECLSPEC int SDLCALL SDL_SetThreadPriority(SDL_ThreadPriority priority);

extern DECLSPEC SDL_TLSID SDLCALL SDL_TLSCreate(void);

extern DECLSPEC void * SDLCALL SDL_TLSGet(SDL_TLSID id);

extern DECLSPEC int SDLCALL SDL_TLSSet(SDL_TLSID id, const void *value, void (SDLCALL *destructor)(void*));

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*SDL_thread_h_*/
#ifndef SDL_atomic_h_
#define SDL_atomic_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "SDL_config.h"
#include "SDL_stdinc.h"

typedef int SDL_SpinLock;

extern DECLSPEC SDL_bool SDLCALL SDL_AtomicTryLock(SDL_SpinLock *lock);
extern DECLSPEC void SDLCALL SDL_AtomicLock(SDL_SpinLock *lock);
extern DECLSPEC void SDLCALL SDL_AtomicUnlock(SDL_SpinLock *lock);

typedef struct { int value; } SDL_atomic_t;

extern DECLSPEC SDL_bool SDLCALL SDL_AtomicCAS(SDL_atomic_t *a, int oldval, int newval);

extern DECLSPEC int SDLCALL SDL_AtomicSet(SDL_atomic_t *a, int v);

extern DECLSPEC int SDLCALL SDL_AtomicGet(SDL_atomic_t *a);

extern DECLSPEC int SDLCALL SDL_AtomicAdd(SDL_atomic_t *a, int v);


#ifndef SDL_AtomicIncRef
#define SDL_AtomicIncRef(a)    SDL_AtomicAdd(a, 1)
#endif

#ifndef SDL_AtomicDecRef
#define SDL_AtomicDecRef(a)    (SDL_AtomicAdd(a, -1) == 1)
#endif


/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*SDL_atomic_h_*/
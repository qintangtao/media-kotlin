#ifndef SDL_error_h_
#define SDL_error_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "SDL_stdinc.h"
#include "SDL_config.h"

extern DECLSPEC int SDLCALL SDL_SetError(const char *fmt, ...);

extern DECLSPEC const char *SDLCALL SDL_GetError(void);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*SDL_error_h_*/
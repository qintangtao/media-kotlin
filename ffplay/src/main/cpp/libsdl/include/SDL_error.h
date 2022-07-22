#ifndef SDL_error_h_
#define SDL_error_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "SDL_stdinc.h"
#include "SDL_config.h"

typedef enum
{
    SDL_ENOMEM,
    SDL_EFREAD,
    SDL_EFWRITE,
    SDL_EFSEEK,
    SDL_UNSUPPORTED,
    SDL_LASTERROR
} SDL_errorcode;

extern DECLSPEC int SDLCALL SDL_SetError(const char *fmt, ...);

extern DECLSPEC const char *SDLCALL SDL_GetError(void);

extern DECLSPEC char * SDLCALL SDL_GetErrorMsg(char *errstr, int maxlen);

extern DECLSPEC void SDLCALL SDL_ClearError(void);

extern DECLSPEC int SDLCALL SDL_Error(SDL_errorcode code);

#define SDL_OutOfMemory()   SDL_Error(SDL_ENOMEM)
#define SDL_Unsupported()   SDL_Error(SDL_UNSUPPORTED)
#define SDL_InvalidParamError(param)    SDL_SetError("Parameter '%s' is invalid", (param))

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*SDL_error_h_*/
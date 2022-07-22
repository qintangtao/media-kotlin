#ifndef SDL_log_h_
#define SDL_log_h_

#include "SDL_stdinc.h"
#include "SDL_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_MAX_LOG_MESSAGE 4096

typedef enum
{
    SDL_LOG_CATEGORY_APPLICATION,
    SDL_LOG_CATEGORY_ERROR,
    SDL_LOG_CATEGORY_ASSERT,
    SDL_LOG_CATEGORY_SYSTEM,
    SDL_LOG_CATEGORY_AUDIO,
    SDL_LOG_CATEGORY_VIDEO,
    SDL_LOG_CATEGORY_RENDER,
    SDL_LOG_CATEGORY_INPUT,
    SDL_LOG_CATEGORY_TEST,

    /* Reserved for future SDL library use */
    SDL_LOG_CATEGORY_RESERVED1,
    SDL_LOG_CATEGORY_RESERVED2,
    SDL_LOG_CATEGORY_RESERVED3,
    SDL_LOG_CATEGORY_RESERVED4,
    SDL_LOG_CATEGORY_RESERVED5,
    SDL_LOG_CATEGORY_RESERVED6,
    SDL_LOG_CATEGORY_RESERVED7,
    SDL_LOG_CATEGORY_RESERVED8,
    SDL_LOG_CATEGORY_RESERVED9,
    SDL_LOG_CATEGORY_RESERVED10,

    /* Beyond this point is reserved for application use, e.g.
       enum {
           MYAPP_CATEGORY_AWESOME1 = SDL_LOG_CATEGORY_CUSTOM,
           MYAPP_CATEGORY_AWESOME2,
           MYAPP_CATEGORY_AWESOME3,
           ...
       };
     */
    SDL_LOG_CATEGORY_CUSTOM
} SDL_LogCategory;


typedef enum
{
    SDL_LOG_PRIORITY_VERBOSE = 1,
    SDL_LOG_PRIORITY_DEBUG,
    SDL_LOG_PRIORITY_INFO,
    SDL_LOG_PRIORITY_WARN,
    SDL_LOG_PRIORITY_ERROR,
    SDL_LOG_PRIORITY_CRITICAL,
    SDL_NUM_LOG_PRIORITIES
} SDL_LogPriority;

extern DECLSPEC void SDLCALL SDL_LogSetAllPriority(SDL_LogPriority priority);

extern DECLSPEC void SDLCALL SDL_LogSetPriority(int category,
                                                SDL_LogPriority priority);

extern DECLSPEC SDL_LogPriority SDLCALL SDL_LogGetPriority(int category);

extern DECLSPEC void SDLCALL SDL_LogResetPriorities(void);

extern DECLSPEC void SDLCALL SDL_Log( const char *fmt, ...) ;

extern DECLSPEC void SDLCALL SDL_LogVerbose(int category,  const char *fmt, ...);

extern DECLSPEC void SDLCALL SDL_LogDebug(int category,  const char *fmt, ...);

extern DECLSPEC void SDLCALL SDL_LogInfo(int category,  const char *fmt, ...);

extern DECLSPEC void SDLCALL SDL_LogWarn(int category,  const char *fmt, ...) ;

extern DECLSPEC void SDLCALL SDL_LogError(int category,  const char *fmt, ...);

extern DECLSPEC void SDLCALL SDL_LogCritical(int category,  const char *fmt, ...);

extern DECLSPEC void SDLCALL SDL_LogMessage(int category,
                                            SDL_LogPriority priority,
                                             const char *fmt, ...) ;

extern DECLSPEC void SDLCALL SDL_LogMessageV(int category,
                                             SDL_LogPriority priority,
                                             const char *fmt, va_list ap);


typedef void (SDLCALL *SDL_LogOutputFunction)(void *userdata, int category, SDL_LogPriority priority, const char *message);

extern DECLSPEC void SDLCALL SDL_LogGetOutputFunction(SDL_LogOutputFunction *callback, void **userdata);

extern DECLSPEC void SDLCALL SDL_LogSetOutputFunction(SDL_LogOutputFunction callback, void *userdata);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*SDL_log_h_*/
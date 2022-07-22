#ifndef SDL_assert_h_
#define SDL_assert_h_

#include "SDL_stdinc.h"
#include "SDL_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SDL_MAX_LOG_MESSAGE 4096

#if defined(_MSC_VER)
/* Don't include intrin.h here because it contains C++ code */
    extern void __cdecl __debugbreak(void);
    #define SDL_TriggerBreakpoint() __debugbreak()
#elif ( (!defined(__NACL__)) && ((defined(__GNUC__) || defined(__clang__)) && (defined(__i386__) || defined(__x86_64__))) )
    #define SDL_TriggerBreakpoint() __asm__ __volatile__ ( "int $3\n\t" )
#elif ( defined(__APPLE__) && defined(__arm64__) )  /* this might work on other ARM targets, but this is a known quantity... */
    #define SDL_TriggerBreakpoint() __asm__ __volatile__ ( "brk #22\n\t" )
#elif defined(__386__) && defined(__WATCOMC__)
    #define SDL_TriggerBreakpoint() { _asm { int 0x03 } }
#elif defined(HAVE_SIGNAL_H) && !defined(__WATCOMC__)
    #include <signal.h>
    #define SDL_TriggerBreakpoint() raise(SIGTRAP)
#else
    /* How do we trigger breakpoints on this platform? */
    #define SDL_TriggerBreakpoint()
#endif

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 supports __func__ as a standard. */
#   define SDL_FUNCTION __func__
#elif ((__GNUC__ >= 2) || defined(_MSC_VER) || defined (__WATCOMC__))
#   define SDL_FUNCTION __FUNCTION__
#else
#   define SDL_FUNCTION "???"
#endif
#define SDL_FILE    __FILE__
#define SDL_LINE    __LINE__

typedef enum
{
    SDL_ASSERTION_RETRY,  /**< Retry the assert immediately. */
    SDL_ASSERTION_BREAK,  /**< Make the debugger trigger a breakpoint. */
    SDL_ASSERTION_ABORT,  /**< Terminate the program. */
    SDL_ASSERTION_IGNORE,  /**< Ignore the assert. */
    SDL_ASSERTION_ALWAYS_IGNORE  /**< Ignore the assert from now on. */
} SDL_AssertState;

typedef struct SDL_AssertData
{
    int always_ignore;
    unsigned int trigger_count;
    const char *condition;
    const char *filename;
    int linenum;
    const char *function;
    const struct SDL_AssertData *next;
} SDL_AssertData;


extern DECLSPEC SDL_AssertState SDLCALL SDL_ReportAssertion(SDL_AssertData *,
                                                             const char *,
                                                             const char *, int);

#define SDL_enabled_assert(condition) \
    do { \
        while ( !(condition) ) { \
            static struct SDL_AssertData sdl_assert_data = { \
                0, 0, #condition, 0, 0, 0, 0 \
            }; \
            const SDL_AssertState sdl_assert_state = SDL_ReportAssertion(&sdl_assert_data, SDL_FUNCTION, SDL_FILE, SDL_LINE); \
            if (sdl_assert_state == SDL_ASSERTION_RETRY) { \
                continue; /* go again. */ \
            } else if (sdl_assert_state == SDL_ASSERTION_BREAK) { \
                SDL_TriggerBreakpoint(); \
            } \
            break; /* not retrying. */ \
        } \
    } while (0)

#define SDL_assert(condition) SDL_enabled_assert(condition)

typedef SDL_AssertState (SDLCALL *SDL_AssertionHandler)(
                                 const SDL_AssertData* data, void* userdata);




/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /*SDL_assert_h_*/
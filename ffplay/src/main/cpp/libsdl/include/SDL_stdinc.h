#ifndef SDL_stdinc_h_
#define SDL_stdinc_h_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "SDL_error.h"
#include "SDL_config.h"

typedef int8_t      Sint8;
typedef uint8_t     Uint8;
typedef int16_t     Sint16;
typedef uint16_t    Uint16;
typedef int32_t     Sint32;
typedef uint32_t    Uint32;
typedef int64_t     Sint64;
typedef uint64_t    Uint64;

#define SDL_FALSE 0
#define SDL_TRUE 1
typedef int SDL_bool;

#define SDL_arraysize(array)    (sizeof(array)/sizeof(array[0]))
#define SDL_TABLESIZE(table)    SDL_arraysize(table)

#define SDL_malloc malloc
#define SDL_calloc calloc
#define SDL_realloc realloc
#define SDL_free free
#define SDL_memset memset
#define SDL_memcpy memcpy
#define SDL_memmove memmove
#define SDL_memcmp memcmp
#define SDL_strlcpy strlcpy
#define SDL_strlcat strlcat
#define SDL_strlen strlen
#define SDL_wcslen wcslen
#define SDL_wcslcpy wcslcpy
#define SDL_wcslcat wcslcat
#define SDL_strdup strdup
#define SDL_wcsdup wcsdup
#define SDL_strchr strchr
#define SDL_strrchr strrchr
#define SDL_strstr strstr
#define SDL_wcsstr wcsstr
#define SDL_strtokr strtok_r
#define SDL_strcmp strcmp
#define SDL_wcscmp wcscmp
#define SDL_strncmp strncmp
#define SDL_wcsncmp wcsncmp
#define SDL_strcasecmp strcasecmp
#define SDL_strncasecmp strncasecmp
#define SDL_sscanf sscanf
#define SDL_vsscanf vsscanf
#define SDL_snprintf snprintf
#define SDL_vsnprintf vsnprintf

#define SDL_atoi atoi

#define SDL_zero(x) SDL_memset(&(x), 0, sizeof((x)))
#define SDL_zerop(x) SDL_memset((x), 0, sizeof(*(x)))
#define SDL_zeroa(x) SDL_memset((x), 0, sizeof((x)))

#define SDL_min(x, y) (((x) < (y)) ? (x) : (y))
#define SDL_max(x, y) (((x) > (y)) ? (x) : (y))

#ifdef __cplusplus
#define SDL_reinterpret_cast(type, expression) reinterpret_cast<type>(expression)
#define SDL_static_cast(type, expression) static_cast<type>(expression)
#define SDL_const_cast(type, expression) const_cast<type>(expression)
#else
#define SDL_reinterpret_cast(type, expression) ((type)(expression))
#define SDL_static_cast(type, expression) ((type)(expression))
#define SDL_const_cast(type, expression) ((type)(expression))
#endif

#define SDL_stack_alloc(type, count)    (type*)SDL_malloc(sizeof(type)*(count))
#define SDL_stack_free(data)            SDL_free(data)

extern DECLSPEC char *SDLCALL SDL_getenv(const char *name);
extern DECLSPEC int SDLCALL SDL_setenv(const char *name, const char *value, int overwrite);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif //SDL_stdinc_h_
#ifndef SDL_atomic_h_
#define SDL_atomic_h_

#ifdef __cplusplus
extern "C" {
#endif

#include "SDL_config.h"
#include "SDL_stdinc.h"

#if defined(__GNUC__) && (defined(__powerpc__) || defined(__ppc__))
    #define SDL_MemoryBarrierRelease()   __asm__ __volatile__ ("lwsync" : : : "memory")
    #define SDL_MemoryBarrierAcquire()   __asm__ __volatile__ ("lwsync" : : : "memory")
#elif defined(__GNUC__) && defined(__aarch64__)
    #define SDL_MemoryBarrierRelease()   __asm__ __volatile__ ("dmb ish" : : : "memory")
    #define SDL_MemoryBarrierAcquire()   __asm__ __volatile__ ("dmb ish" : : : "memory")
#elif defined(__GNUC__) && defined(__arm__)
    #if 0 /* defined(__LINUX__) || defined(__ANDROID__) */
        /* Information from:
           https://chromium.googlesource.com/chromium/chromium/+/trunk/base/atomicops_internals_arm_gcc.h#19

           The Linux kernel provides a helper function which provides the right code for a memory barrier,
           hard-coded at address 0xffff0fa0
        */
        typedef void (*SDL_KernelMemoryBarrierFunc)();
        #define SDL_MemoryBarrierRelease()	((SDL_KernelMemoryBarrierFunc)0xffff0fa0)()
        #define SDL_MemoryBarrierAcquire()	((SDL_KernelMemoryBarrierFunc)0xffff0fa0)()
    #elif 0 /* defined(__QNXNTO__) */
        #include <sys/cpuinline.h>

        #define SDL_MemoryBarrierRelease()   __cpu_membarrier()
        #define SDL_MemoryBarrierAcquire()   __cpu_membarrier()
    #else

        #if defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__) || defined(__ARM_ARCH_8A__)

            #define SDL_MemoryBarrierRelease()   __asm__ __volatile__ ("dmb ish" : : : "memory")
            #define SDL_MemoryBarrierAcquire()   __asm__ __volatile__ ("dmb ish" : : : "memory")

        #elif defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6T2__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_5TE__)

            #ifdef __thumb__
            /* The mcr instruction isn't available in thumb mode, use real functions */
            #define SDL_MEMORY_BARRIER_USES_FUNCTION
            #define SDL_MemoryBarrierRelease()   SDL_MemoryBarrierReleaseFunction()
            #define SDL_MemoryBarrierAcquire()   SDL_MemoryBarrierAcquireFunction()
            #else
            #define SDL_MemoryBarrierRelease()   __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 5" : : "r"(0) : "memory")
            #define SDL_MemoryBarrierAcquire()   __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 5" : : "r"(0) : "memory")
            #endif /* __thumb__ */

        #else
            #define SDL_MemoryBarrierRelease()   __asm__ __volatile__ ("" : : : "memory")
            #define SDL_MemoryBarrierAcquire()   __asm__ __volatile__ ("" : : : "memory")
        #endif /* __LINUX__ || __ANDROID__ */

    #endif /* __GNUC__ && __arm__ */

#else

    #if (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x5120))
        /* This is correct for all CPUs on Solaris when using Solaris Studio 12.1+. */
        #include <mbarrier.h>
        #define SDL_MemoryBarrierRelease()  __machine_rel_barrier()
        #define SDL_MemoryBarrierAcquire()  __machine_acq_barrier()
    #else
        /* This is correct for the x86 and x64 CPUs, and we'll expand this over time. */
        #define SDL_MemoryBarrierRelease()  SDL_CompilerBarrier()
        #define SDL_MemoryBarrierAcquire()  SDL_CompilerBarrier()
    #endif

#endif

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
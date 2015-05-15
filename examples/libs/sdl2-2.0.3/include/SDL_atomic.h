/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

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

/**
 * \file SDL_atomic.h
 *
 * Atomic operations.
 *
 * IMPORTANT:
 * If you are not an expert in concurrent lockless programming, you should
 * only be using the atomic lock and reference counting functions in this
 * file.  In all other cases you should be protecting your data structures
 * with full mutexes.
 *
 * The list of "safe" functions to use are:
 *  SDL_AtomicLock()
 *  SDL_AtomicUnlock()
 *  SDL_AtomicIncRef()
 *  SDL_AtomicDecRef()
 *
 * Seriously, here be dragons!
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 * You can find out a little more about lockless programming and the
 * subtle issues that can arise here:
 * http://msdn.microsoft.com/en-us/library/ee418650%28v=vs.85%29.aspx
 *
 * There's also lots of good information here:
 * http://www.1024cores.net/home/lock-free-algorithms
 * http://preshing.com/
 *
 * These operations may or may not actually be implemented using
 * processor specific atomic operations. When possible they are
 * implemented as true processor specific atomic operations. When that
 * is not possible the are implemented using locks that *do* use the
 * available atomic operations.
 *
 * All of the atomic operations that modify memory are full memory barriers.
 */

#ifndef _SDL_atomic_h_
#define _SDL_atomic_h_

#include "SDL_stdinc.h"
#include "SDL_platform.h"

#include "begin_code.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * \name SDL AtomicLock
 *
 * The atomic locks are efficient spinlocks using CPU instructions,
 * but are vulnerable to starvation and can spin forever if a thread
 * holding a lock has been terminated.  For this reason you should
 * minimize the code executed inside an atomic lock and never do
 * expensive things like API or system calls while holding them.
 *
 * The atomic locks are not safe to lock recursively.
 *
 * Porting Note:
 * The spin lock functions and type are required and can not be
 * emulated because they are used in the atomic emulation code.
 */
/* @{ */

typedef int SDL_SpinLock;

/**
 * \brief Try to lock a spin lock by setting it to a non-zero value.
 *
 * \param lock Points to the lock.
 *
 * \return SDL_TRUE if the lock succeeded, SDL_FALSE if the lock is already held.
 */
extern DECLSPEC SDL_bool SDLCALL SDL_AtomicTryLock(SDL_SpinLock *lock);

/**
 * \brief Lock a spin lock by setting it to a non-zero value.
 *
 * \param lock Points to the lock.
 */
extern DECLSPEC void SDLCALL SDL_AtomicLock(SDL_SpinLock *lock);

/**
 * \brief Unlock a spin lock by setting it to 0. Always returns immediately
 *
 * \param lock Points to the lock.
 */
extern DECLSPEC void SDLCALL SDL_AtomicUnlock(SDL_SpinLock *lock);

/* @} *//* SDL AtomicLock */


/**
 * The compiler barrier prevents the compiler from reordering
 * reads and writes to globally visible variables across the call.
 */
#if defined(_MSC_VER) && (_MSC_VER > 1200)
void _ReadWriteBarrier(void);
#pragma intrinsic(_ReadWriteBarrier)
#define SDL_CompilerBarrier()   _ReadWriteBarrier()
#elif defined(__GNUC__)
#define SDL_CompilerBarrier()   __asm__ __volatile__ ("" : : : "memory")
#else
#define SDL_CompilerBarrier()   \
{ SDL_SpinLock _tmp = 0; SDL_AtomicLock(&_tmp); SDL_AtomicUnlock(&_tmp); }
#endif

/**
 * Memory barriers are designed to prevent reads and writes from being
 * reordered by the compiler and being seen out of order on multi-core CPUs.
 *
 * A typical pattern would be for thread A to write some data and a flag,
 * and for thread B to read the flag and get the data. In this case you
 * would insert a release barrier between writing the data and the flag,
 * guaranteeing that the data write completes no later than the flag is
 * written, and you would insert an acquire barrier between reading the
 * flag and reading the data, to ensure that all the reads associated
 * with the flag have completed.
 *
 * In this pattern you should always see a release barrier paired with
 * an acquire barrier and you should gate the data reads/writes with a
 * single flag variable.
 *
 * For more information on these semantics, take a look at the blog post:
 * http://preshing.com/20120913/acquire-and-release-semantics
 */
#if defined(__GNUC__) && (defined(__powerpc__) || defined(__ppc__))
#define SDL_MemoryBarrierRelease()   __asm__ __volatile__ ("lwsync" : : : "memory")
#define SDL_MemoryBarrierAcquire()   __asm__ __volatile__ ("lwsync" : : : "memory")
#elif defined(__GNUC__) && defined(__arm__)
#if defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7EM__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
#define SDL_MemoryBarrierRelease()   __asm__ __volatile__ ("dmb ish" : : : "memory")
#define SDL_MemoryBarrierAcquire()   __asm__ __volatile__ ("dmb ish" : : : "memory")
#elif defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6T2__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__)
#ifdef __thumb__
/* The mcr instruction isn't available in thumb mode, use real functions */
extern DECLSPEC void SDLCALL SDL_MemoryBarrierRelease();
extern DECLSPEC void SDLCALL SDL_MemoryBarrierAcquire();
#else
#define SDL_MemoryBarrierRelease()   __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 5" : : "r"(0) : "memory")
#define SDL_MemoryBarrierAcquire()   __asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 5" : : "r"(0) : "memory")
#endif /* __thumb__ */
#else
#define SDL_MemoryBarrierRelease()   __asm__ __volatile__ ("" : : : "memory")
#define SDL_MemoryBarrierAcquire()   __asm__ __volatile__ ("" : : : "memory")
#endif /* __GNUC__ && __arm__ */
#else
/* This is correct for the x86 and x64 CPUs, and we'll expand this over time. */
#define SDL_MemoryBarrierRelease()  SDL_CompilerBarrier()
#define SDL_MemoryBarrierAcquire()  SDL_CompilerBarrier()
#endif

/**
 * \brief A type representing an atomic integer value.  It is a struct
 *        so people don't accidentally use numeric operations on it.
 */
typedef struct { int value; } SDL_atomic_t;

/**
 * \brief Set an atomic variable to a new value if it is currently an old value.
 *
 * \return SDL_TRUE if the atomic variable was set, SDL_FALSE otherwise.
 *
 * \note If you don't know what this function is for, you shouldn't use it!
*/
extern DECLSPEC SDL_bool SDLCALL SDL_AtomicCAS(SDL_atomic_t *a, int oldval, int newval);

/**
 * \brief Set an atomic variable to a value.
 *
 * \return The previous value of the atomic variable.
 */
extern DECLSPEC int SDLCALL SDL_AtomicSet(SDL_atomic_t *a, int v);

/**
 * \brief Get the value of an atomic variable
 */
extern DECLSPEC int SDLCALL SDL_AtomicGet(SDL_atomic_t *a);

/**
 * \brief Add to an atomic variable.
 *
 * \return The previous value of the atomic variable.
 *
 * \note This same style can be used for any number operation
 */
extern DECLSPEC int SDLCALL SDL_AtomicAdd(SDL_atomic_t *a, int v);

/**
 * \brief Increment an atomic variable used as a reference count.
 */
#ifndef SDL_AtomicIncRef
#define SDL_AtomicIncRef(a)    SDL_AtomicAdd(a, 1)
#endif

/**
 * \brief Decrement an atomic variable used as a reference count.
 *
 * \return SDL_TRUE if the variable reached zero after decrementing,
 *         SDL_FALSE otherwise
 */
#ifndef SDL_AtomicDecRef
#define SDL_AtomicDecRef(a)    (SDL_AtomicAdd(a, -1) == 1)
#endif

/**
 * \brief Set a pointer to a new value if it is currently an old value.
 *
 * \return SDL_TRUE if the pointer was set, SDL_FALSE otherwise.
 *
 * \note If you don't know what this function is for, you shouldn't use it!
*/
extern DECLSPEC SDL_bool SDLCALL SDL_AtomicCASPtr(void **a, void *oldval, void *newval);

/**
 * \brief Set a pointer to a value atomically.
 *
 * \return The previous value of the pointer.
 */
extern DECLSPEC void* SDLCALL SDL_AtomicSetPtr(void **a, void* v);

/**
 * \brief Get the value of a pointer atomically.
 */
extern DECLSPEC void* SDLCALL SDL_AtomicGetPtr(void **a);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#include "close_code.h"

#endif /* _SDL_atomic_h_ */

/* vi: set ts=4 sw=4 expandtab: */

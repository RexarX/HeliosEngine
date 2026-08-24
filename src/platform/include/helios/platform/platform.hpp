#pragma once

// Platform-specific API macros
#ifdef HELIOS_PLATFORM_WINDOWS
#ifdef HELIOS_BUILD_SHARED
#define HELIOS_API __declspec(dllexport)
#else
#define HELIOS_API __declspec(dllimport)
#endif
#elifdef HELIOS_PLATFORM_LINUX
#ifdef HELIOS_BUILD_SHARED
#define HELIOS_API __attribute__((visibility("default")))
#else
#define HELIOS_API
#endif
#else
#define HELIOS_API
#endif

#ifdef HELIOS_PLATFORM_WINDOWS
#define HELIOS_EXPORT __declspec(dllexport)
#elifdef HELIOS_PLATFORM_LINUX
#define HELIOS_EXPORT __attribute__((visibility("default")))
#else
#define HELIOS_EXPORT
#endif

// TODO: Replace with std::breakpoint() from <debugging> in C++26
// https://en.cppreference.com/w/cpp/utility/breakpoint.html

// NOLINTBEGIN(hicpp-no-assembler)

// MSVC: Use the intrinsic which maps to the appropriate inlined assembly
#if defined(_MSC_VER) && (_MSC_VER >= 1300)
#define HELIOS_DEBUG_BREAK() __debugbreak()

// ARM64 on Apple: trap so CI/ctest terminate instead of swallowing SIGINT
#elif defined(__arm64__) && defined(__APPLE__)
#define HELIOS_DEBUG_BREAK() __builtin_debugtrap()

// ARM64 with GCC/Clang: Use brk instruction
#elif defined(__arm64__) && (defined(__GNUC__) || defined(__clang__))
#define HELIOS_DEBUG_BREAK() __asm__("brk 0")

// ARM (32-bit) on Apple: Use trap instruction
#elif defined(__arm__) && defined(__APPLE__)
#define HELIOS_DEBUG_BREAK() __asm__("trap")

// ARM (32-bit) with GCC/Clang: Use bkpt instruction
#elif defined(__arm__) && (defined(__GNUC__) || defined(__clang__))
#define HELIOS_DEBUG_BREAK() __asm__("bkpt 0")

// ARM with Arm Compiler: Use breakpoint intrinsic
#elif defined(__arm__) && defined(__ARMCC_VERSION)
#define HELIOS_DEBUG_BREAK() __breakpoint(0)

// x86/x86_64 with Clang/GCC: Use int3 instruction (AT&T syntax)
#elif (defined(__x86_64__) || defined(__i386__)) && \
    (defined(__GNUC__) || defined(__clang__))
#define HELIOS_DEBUG_BREAK() __asm__("int3")

// x86/x86_64 with MSVC: Use int3 instruction (Intel syntax)
#elif (defined(_M_X64) || defined(_M_IX86)) && defined(_MSC_VER)
#define HELIOS_DEBUG_BREAK() __asm int 3

// PowerPC: Trigger exception via opcode 0x00000000
#elif defined(__powerpc__) || defined(__ppc__) || defined(_ARCH_PPC)
#define HELIOS_DEBUG_BREAK() __asm__(".long 0")

// RISC-V: Use ebreak instruction
#elif defined(__riscv) || defined(__riscv__)
#define HELIOS_DEBUG_BREAK() __asm__("ebreak")

// WebAssembly: No breakpoint support, use unreachable
#elif defined(__wasm__)
#define HELIOS_DEBUG_BREAK() __asm__("unreachable")

// Fallback: Use compiler builtin trap (works on most modern compilers)
#elif defined(__has_builtin) && __has_builtin(__builtin_trap)
#define HELIOS_DEBUG_BREAK() __builtin_trap()

// NOLINTEND(hicpp-no-assembler)

// Last resort: Use signal-based approach
#else
#include <csignal>
#define HELIOS_DEBUG_BREAK() ::std::raise(SIGTRAP)
#endif

/**
 * @brief Inserts a CPU pause hint for busy-wait loops.
 * @details No-op on unknown architectures.
 *
 * Adapted from SDL3 `SDL_CPUPauseInstruction` (`SDL_atomic.h`).
 *
 * Simple DirectMedia Layer
 * Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * Altered: renamed to `HELIOS_PAUSE_CPU`
 */
// NOLINTBEGIN(hicpp-no-assembler)
#if (defined(__GNUC__) || defined(__clang__)) && \
    (defined(__i386__) || defined(__x86_64__)) && !defined(__arm64ec__)
#define HELIOS_PAUSE_CPU() __asm__ __volatile__("pause\n")
#elif (defined(__arm__) && defined(__ARM_ARCH) && __ARM_ARCH >= 7) || \
    defined(__aarch64__)
#define HELIOS_PAUSE_CPU() __asm__ __volatile__("yield" ::: "memory")
#elif defined(__powerpc__) || defined(__powerpc64__)
#define HELIOS_PAUSE_CPU() __asm__ __volatile__("or 27,27,27")
#elif defined(__riscv) && __riscv_xlen == 64
#define HELIOS_PAUSE_CPU() \
  __asm__ __volatile__(".insn i 0x0F, 0, x0, x0, 0x010")
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#define HELIOS_PAUSE_CPU() _mm_pause()
#elif defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64))
#define HELIOS_PAUSE_CPU() __yield()
#else
#define HELIOS_PAUSE_CPU()
#endif
// NOLINTEND(hicpp-no-assembler)

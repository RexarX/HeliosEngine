#pragma once

#include <helios/config.hpp>

#if HELIOS_MODULE_HEADER_IMPORT
import helios.profile;
#define HELIOS_MODULE_CONSUMER_SHIM
#endif

#ifndef HELIOS_MODULE_CONSUMER_SHIM
#ifndef HELIOS_BUILDING_MODULE
#include <helios/cstring_view.hpp>

#include <cstddef>
#include <optional>
#include <source_location>
#endif
#include <helios/profile/profiler.hpp>

HELIOS_MODULE_EXPORT
namespace helios::profile {

/**
 * @brief Profile an allocation of memory.
 * @param ptr The pointer to the allocated memory
 * @param size The size of the allocated memory
 * @param name The name of the allocation, used for grouping in the profiler
 * @param depth The depth of the allocation, used for grouping in the profiler
 * @param loc The source location of the allocation, used for grouping in the
 * profiler
 */
inline void Alloc(
    const void* ptr, size_t size, std::optional<CStringView> name, int depth,
    std::source_location loc = std::source_location::current()) noexcept {
  Profiler::Instance().Alloc(ptr, size, name, depth, loc);
}

/**
 * @brief Profile a deallocation of memory.
 * @param ptr The pointer to the deallocated memory
 * @param name The name of the deallocation, used for grouping in the profiler
 * @param depth The depth of the deallocation, used for grouping in the profiler
 * @param loc The source location of the deallocation, used for grouping in the
 * profiler
 */
inline void Free(
    const void* ptr, std::optional<CStringView> name, int depth,
    std::source_location loc = std::source_location::current()) noexcept {
  Profiler::Instance().Free(ptr, name, depth, loc);
}

/**
 * @brief Profile memory discard.
 * @param name The name of the memory discard, used for grouping in the profiler
 */
inline void MemoryDiscard(CStringView name) noexcept {
  Profiler::Instance().MemoryDiscard(name);
}

/**
 * @brief Profile memory discard with depth.
 * @param name The name of the memory discard, used for grouping in the profiler
 * @param depth The depth of the memory discard, used for grouping in the
 * profiler
 */
inline void MemoryDiscardS(CStringView name, int depth) noexcept {
  Profiler::Instance().MemoryDiscard(name, depth);
}

}  // namespace helios::profile
#endif  // HELIOS_MODULE_CONSUMER_SHIM

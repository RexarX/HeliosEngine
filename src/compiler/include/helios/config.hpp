#pragma once

/**
 * @file config.hpp
 * @brief Dual header / C++20 named-module wrapping macros.
 *
 * Named modules use Boost-style `export extern "C++"` so declarations keep
 * classic C++ ABI (global module / language linkage). Interface units wrap
 * their public headers with `HELIOS_BEGIN_MODULE_EXPORT` /
 * `HELIOS_END_MODULE_EXPORT`. Implementation `.cpp` files stay classic TUs
 * (`HELIOS_MODULE_IMPLEMENTATION`).
 *
 * Macro-only headers (`compiler.hpp`, `platform.hpp`, `utils/macro.hpp`) stay
 * textual: named modules cannot export macros.
 */

// Empty since `export` on individual declarations would attach them to the
// module purview and change ABI.
#define HELIOS_MODULE_EXPORT

#if defined(HELIOS_ENABLE_CPP_MODULES) && defined(HELIOS_BUILDING_MODULE)
#define HELIOS_BEGIN_MODULE_EXPORT export extern "C++" {
#define HELIOS_END_MODULE_EXPORT }
#else
#define HELIOS_BEGIN_MODULE_EXPORT
#define HELIOS_END_MODULE_EXPORT
#endif

// Include-to-import is off. MSVC without `import std` rejects `#include` of
// standard headers after `import` (C2572). A shared consumer prelude would
// hide that, but it also hides includes from the files that use them.
// `#include <helios/...>` stays textual; `import helios.X` uses the BMI.
// Mixing works because interface units wrap headers in
// `HELIOS_BEGIN_MODULE_EXPORT` (classic ABI).
#define HELIOS_MODULE_HEADER_IMPORT 0

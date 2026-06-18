#pragma once

#define HELIOS_BIT(x) (1 << (x))

// Stringify macros
#define HELIOS_STRINGIFY_IMPL(x) #x
#define HELIOS_STRINGIFY(x) HELIOS_STRINGIFY_IMPL(x)

// Concatenation macros
#define HELIOS_CONCAT_IMPL(a, b) a##b
#define HELIOS_CONCAT(a, b) HELIOS_CONCAT_IMPL(a, b)

// Anonymous variable generation
#define HELIOS_ANONYMOUS_VAR(prefix) HELIOS_CONCAT(prefix, __LINE__)

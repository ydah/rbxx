#pragma once

#define RBXX_VERSION_MAJOR 0
#define RBXX_VERSION_MINOR 1
#define RBXX_VERSION_PATCH 0
#define RBXX_VERSION "0.1.0"

namespace rbxx {

/// @brief The rbxx semantic version string.
/// @code auto version = rbxx::version; @endcode
inline constexpr const char* version = RBXX_VERSION;

} // namespace rbxx

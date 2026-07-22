#pragma once

namespace rbxx::op {

/// @brief Metadata for mapping a C++ callable to a Ruby operator method.
struct name {
  const char* ruby_name;
  bool include_comparable = false;
};

/// @name Ruby operator names
/// @{
inline constexpr name add{"+"};
inline constexpr name subtract{"-"};
inline constexpr name multiply{"*"};
inline constexpr name divide{"/"};
inline constexpr name modulo{"%"};
inline constexpr name equal{"=="};
inline constexpr name compare{"<=>", true};
inline constexpr name less{"<"};
inline constexpr name greater{">"};
inline constexpr name index{"[]"};
inline constexpr name index_set{"[]="};
inline constexpr name left_shift{"<<"};
inline constexpr name to_s{"to_s"};
inline constexpr name inspect{"inspect"};
inline constexpr name hash{"hash"};
inline constexpr name call{"call"};
/// @}

} // namespace rbxx::op

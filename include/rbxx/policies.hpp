#pragma once

#include <cstddef>

namespace rbxx {

namespace policy {

enum class kind { automatic, copy, take, reference, shared };

struct return_value_policy {
  kind value;
};

inline constexpr return_value_policy automatic{kind::automatic};
inline constexpr return_value_policy copy{kind::copy};
inline constexpr return_value_policy take{kind::take};
inline constexpr return_value_policy reference{kind::reference};
inline constexpr return_value_policy shared{kind::shared};

} // namespace policy

/// @brief Keeps one Ruby object alive for as long as another remains reachable.
/// @details Index 0 is the return value, 1 is self, and 2 onward are arguments.
/// @code .def("child", &Owner::child, policy::reference, keep_alive<0, 1>()) @endcode
template <std::size_t Nurse, std::size_t Patient> struct keep_alive {
  static constexpr std::size_t nurse = Nurse;
  static constexpr std::size_t patient = Patient;
};

} // namespace rbxx

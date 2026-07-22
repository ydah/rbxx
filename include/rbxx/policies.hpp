#pragma once

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

} // namespace rbxx

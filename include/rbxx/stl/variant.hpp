#pragma once

#include <rbxx/stl/detail.hpp>

#include <variant>

namespace rbxx {
namespace detail {
template <typename... Items>
struct is_non_bindable_class<std::variant<Items...>> : std::true_type {};

template <typename Variant, std::size_t Index = 0> Variant load_variant(value input) {
  if constexpr (Index == std::variant_size_v<Variant>) {
    throw_type_error("one of the std::variant alternatives", input);
  } else {
    using alternative = std::variant_alternative_t<Index, Variant>;
    if (type_caster<alternative>::matches(input)) {
      return Variant{std::in_place_index<Index>, from_ruby<alternative>(input)};
    }
    return load_variant<Variant, Index + 1U>(input);
  }
}
} // namespace detail

template <typename... Items> struct type_caster<std::variant<Items...>> {
  using variant_type = std::variant<Items...>;
  static constexpr std::string_view name = "variant";
  static variant_type load(value input) { return detail::load_variant<variant_type>(input); }
  static value dump(const variant_type& input) {
    return std::visit([](const auto& selected) { return to_ruby(selected); }, input);
  }
  static bool matches(value input) noexcept { return (type_caster<Items>::matches(input) || ...); }
};

} // namespace rbxx

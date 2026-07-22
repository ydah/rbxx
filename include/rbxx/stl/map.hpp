#pragma once

#include <rbxx/stl/detail.hpp>

#include <map>
#include <unordered_map>

namespace rbxx {
namespace detail {
template <typename Key, typename Mapped, typename Compare, typename Allocator>
struct is_non_bindable_class<std::map<Key, Mapped, Compare, Allocator>> : std::true_type {};
template <typename Key, typename Mapped, typename Hash, typename Equal, typename Allocator>
struct is_non_bindable_class<std::unordered_map<Key, Mapped, Hash, Equal, Allocator>>
    : std::true_type {};

template <typename Map> Map load_map(value input) {
  VALUE pairs = hash_pairs(coerce_hash(input));
  Map result;
  const long length = RARRAY_LEN(pairs);
  for (long index = 0; index < length; ++index) {
    VALUE pair = RARRAY_AREF(pairs, index);
    using key_type = typename Map::key_type;
    using mapped_type = typename Map::mapped_type;
    result.emplace(from_ruby<key_type>(value{RARRAY_AREF(pair, 0)}),
                   from_ruby<mapped_type>(value{RARRAY_AREF(pair, 1)}));
  }
  return result;
}
} // namespace detail

template <typename Key, typename Mapped, typename Compare, typename Allocator>
struct type_caster<std::map<Key, Mapped, Compare, Allocator>> {
  using map_type = std::map<Key, Mapped, Compare, Allocator>;
  static constexpr std::string_view name = "Hash";
  static map_type load(value input) { return detail::load_map<map_type>(input); }
  static value dump(const map_type& input) { return detail::dump_hash_range(input); }
  static bool matches(value input) noexcept { return input.is_hash(); }
};

template <typename Key, typename Mapped, typename Hash, typename Equal, typename Allocator>
struct type_caster<std::unordered_map<Key, Mapped, Hash, Equal, Allocator>> {
  using map_type = std::unordered_map<Key, Mapped, Hash, Equal, Allocator>;
  static constexpr std::string_view name = "Hash";
  static map_type load(value input) { return detail::load_map<map_type>(input); }
  static value dump(const map_type& input) { return detail::dump_hash_range(input); }
  static bool matches(value input) noexcept { return input.is_hash(); }
};

} // namespace rbxx

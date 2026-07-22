#pragma once

#include <rbxx/type_caster.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace rbxx {

struct argument_spec {
  std::string name;
  bool keyword = false;
  bool has_default = false;
  object default_value;
};

/// @brief Declares a named positional argument and an optional default.
/// @code function.def("add", fn, rbxx::arg("delta") = 1); @endcode
class arg {
public:
  explicit arg(const char* name) : name_(name) {}
  operator argument_spec() const { return argument_spec{name_, false, false, {}}; }

  template <typename T> argument_spec operator=(T&& default_value) const {
    return argument_spec{name_, false, true, object{to_ruby(std::forward<T>(default_value))}};
  }

private:
  std::string name_;
};

/// @brief Declares a keyword argument and an optional default.
/// @code function.def("open", fn, rbxx::kwarg("mode") = "fast"); @endcode
class kwarg {
public:
  explicit kwarg(const char* name) : name_(name) {}
  operator argument_spec() const { return argument_spec{name_, true, false, {}}; }

  template <typename T> argument_spec operator=(T&& default_value) const {
    return argument_spec{name_, true, true, object{to_ruby(std::forward<T>(default_value))}};
  }

private:
  std::string name_;
};

/// @brief Required Ruby block passed to a bound C++ callable.
class block {
public:
  explicit block(value callable) : callable_(callable) {}

  [[nodiscard]] value get() const noexcept { return callable_.get(); }

  template <typename Return = value, typename... Args> Return call(Args&&... args) const {
    std::array<VALUE, sizeof...(Args)> arguments{to_ruby(std::forward<Args>(args)).raw()...};
    VALUE result = protect(rb_funcallv, callable_.raw(), rb_intern("call"),
                           static_cast<int>(sizeof...(Args)), arguments.data());
    if constexpr (std::is_same_v<Return, value>) {
      return value{result};
    } else {
      return from_ruby<Return>(value{result});
    }
  }

private:
  object callable_;
};

/// @brief Optional Ruby block passed to a bound C++ callable.
class optional_block {
public:
  optional_block() = default;
  explicit optional_block(value callable) : callable_(callable) {}

  [[nodiscard]] explicit operator bool() const noexcept { return !callable_.is_nil(); }
  [[nodiscard]] value get() const noexcept { return callable_.get(); }

  template <typename Return = value, typename... Args> Return call(Args&&... args) const {
    if (!*this) {
      throw std::invalid_argument("rbxx: optional block is not present");
    }
    return block{callable_.get()}.template call<Return>(std::forward<Args>(args)...);
  }

private:
  object callable_;
};

/// @brief Remaining positional Ruby arguments.
class args {
public:
  explicit args(std::vector<value> values) : values_(std::move(values)) {}
  [[nodiscard]] const std::vector<value>& values() const noexcept { return values_; }
  [[nodiscard]] std::size_t size() const noexcept { return values_.size(); }
  [[nodiscard]] value operator[](std::size_t index) const { return values_.at(index); }

private:
  std::vector<value> values_;
};

template <> struct type_caster<block> {
  static block load(value input) { return block{input}; }
  static value dump(const block& input) noexcept { return input.get(); }
  static bool matches(value input) noexcept { return RTEST(rb_obj_is_proc(input.raw())); }
};

template <> struct type_caster<optional_block> {
  static optional_block load(value input) {
    return input.is_nil() ? optional_block{} : optional_block{input};
  }
  static value dump(const optional_block& input) noexcept {
    return input ? input.get() : value{Qnil};
  }
  static bool matches(value input) noexcept {
    return input.is_nil() || RTEST(rb_obj_is_proc(input.raw()));
  }
};

template <> struct type_caster<args> {
  static args load(value input) { return args{{input}}; }
  static value dump(const args& input) {
    return value{protect([&input] {
      VALUE result = rb_ary_new_capa(static_cast<long>(input.size()));
      for (value element : input.values()) {
        rb_ary_push(result, element.raw());
      }
      return result;
    })};
  }
  static bool matches(value) noexcept { return true; }
};

namespace detail {

template <typename Argument> auto load_argument(value input) {
  using loaded_type = decltype(from_ruby<Argument>(input));
  if constexpr (std::is_lvalue_reference_v<loaded_type>) {
    return std::ref(from_ruby<Argument>(input));
  } else {
    return from_ruby<Argument>(input);
  }
}

template <typename T>
inline constexpr bool is_block_argument_v = std::is_same_v<std::remove_cvref_t<T>, block>;
template <typename T>
inline constexpr bool is_optional_block_argument_v =
    std::is_same_v<std::remove_cvref_t<T>, optional_block>;
template <typename T>
inline constexpr bool is_rest_argument_v = std::is_same_v<std::remove_cvref_t<T>, args>;
template <typename T>
inline constexpr bool is_special_argument_v =
    is_block_argument_v<T> || is_optional_block_argument_v<T> || is_rest_argument_v<T>;

template <typename Tuple, std::size_t... Index>
consteval std::size_t normal_argument_count(std::index_sequence<Index...>) {
  return (std::size_t{0} + ... +
          (is_special_argument_v<std::tuple_element_t<Index, Tuple>> ? 0U : 1U));
}

template <typename Tuple> consteval std::size_t normal_argument_count() {
  return normal_argument_count<Tuple>(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

inline std::vector<argument_spec> make_argument_specs() { return {}; }

template <typename... Specs> std::vector<argument_spec> make_argument_specs(Specs&&... specs) {
  static_assert((std::is_convertible_v<Specs, argument_spec> && ...),
                "rbxx: binding options must be arg(...) or kwarg(...)");
  std::vector<argument_spec> result;
  result.reserve(sizeof...(Specs));
  (result.emplace_back(static_cast<argument_spec>(std::forward<Specs>(specs))), ...);
  return result;
}

struct parsed_arguments {
  std::vector<value> slots;
  std::vector<value> rest;
  value block_value{Qnil};
};

template <typename Tuple> class argument_parser {
public:
  static constexpr std::size_t arity = std::tuple_size_v<Tuple>;

  explicit argument_parser(std::vector<argument_spec> specs) : specs_(std::move(specs)) {
    constexpr std::size_t normal = normal_argument_count<Tuple>();
    if (specs_.empty()) {
      specs_.reserve(normal);
      for (std::size_t index = 0; index < normal; ++index) {
        specs_.push_back(argument_spec{"arg" + std::to_string(index + 1U), false, false, {}});
      }
    }
    if (specs_.size() != normal) {
      throw std::invalid_argument("rbxx: argument annotations must match non-block parameters");
    }
  }

  [[nodiscard]] parsed_arguments parse(int argc, const VALUE* argv) const {
    parsed_arguments parsed;
    parsed.slots.resize(arity, value{Qundef});

    int positional_count = argc;
    VALUE keywords = Qnil;
    if (argc > 0 && rb_keyword_given_p()) {
      keywords = argv[argc - 1];
      --positional_count;
    }

    std::vector<VALUE> keyword_values(specs_.size(), Qundef);
    read_keywords(keywords, keyword_values);
    fill_slots(parsed, positional_count, argv, keyword_values, std::make_index_sequence<arity>{});
    read_block(parsed);
    return parsed;
  }

  [[nodiscard]] bool configured() const noexcept {
    return std::any_of(specs_.begin(), specs_.end(),
                       [](const argument_spec& spec) { return spec.keyword || spec.has_default; });
  }

private:
  void read_keywords(VALUE keywords, std::vector<VALUE>& values) const {
    std::vector<ID> required;
    std::vector<ID> optional;
    std::vector<std::size_t> required_order;
    std::vector<std::size_t> optional_order;
    for (std::size_t index = 0; index < specs_.size(); ++index) {
      const auto& spec = specs_[index];
      if (!spec.keyword) {
        continue;
      }
      ID id = protect(rb_intern, spec.name.c_str());
      (spec.has_default ? optional : required).push_back(id);
      (spec.has_default ? optional_order : required_order).push_back(index);
    }

    if (required.empty() && optional.empty()) {
      if (!NIL_P(keywords) && RHASH_SIZE(keywords) != 0) {
        throw ruby_error(make_exception(rb_eArgError, "rbxx: unknown keyword arguments"));
      }
      return;
    }

    if (NIL_P(keywords)) {
      keywords = protect(rb_hash_new);
    }
    std::vector<ID> table = required;
    table.insert(table.end(), optional.begin(), optional.end());
    std::vector<std::size_t> order = required_order;
    order.insert(order.end(), optional_order.begin(), optional_order.end());
    std::vector<VALUE> extracted(table.size(), Qundef);
    protect([&] {
      return rb_get_kwargs(keywords, table.data(), static_cast<int>(required.size()),
                           static_cast<int>(optional.size()), extracted.data());
    });
    for (std::size_t index = 0; index < order.size(); ++index) {
      values[order[index]] = extracted[index];
    }
  }

  template <std::size_t... Index>
  void fill_slots(parsed_arguments& parsed, int positional_count, const VALUE* argv,
                  const std::vector<VALUE>& keyword_values, std::index_sequence<Index...>) const {
    std::size_t spec_index = 0;
    int positional_index = 0;
    (fill_slot<Index>(parsed, positional_count, positional_index, argv, spec_index, keyword_values),
     ...);
    if (positional_index < positional_count && !has_rest(std::make_index_sequence<arity>{})) {
      throw ruby_error(make_exception(rb_eArgError, "rbxx: too many positional arguments"));
    }
  }

  template <std::size_t Index>
  void fill_slot(parsed_arguments& parsed, int positional_count, int& positional_index,
                 const VALUE* argv, std::size_t& spec_index,
                 const std::vector<VALUE>& keyword_values) const {
    using argument = std::tuple_element_t<Index, Tuple>;
    if constexpr (is_rest_argument_v<argument>) {
      while (positional_index < positional_count) {
        parsed.rest.emplace_back(argv[positional_index++]);
      }
    } else if constexpr (!is_block_argument_v<argument> &&
                         !is_optional_block_argument_v<argument>) {
      const argument_spec& spec = specs_[spec_index];
      VALUE selected = Qundef;
      if (spec.keyword) {
        selected = keyword_values[spec_index];
      } else if (positional_index < positional_count) {
        selected = argv[positional_index++];
      }
      if (selected == Qundef && spec.has_default) {
        selected = spec.default_value.raw();
      }
      if (selected == Qundef) {
        std::string message = "rbxx: missing argument ";
        message += spec.name;
        throw ruby_error(make_exception(rb_eArgError, message.c_str()));
      }
      parsed.slots[Index] = value{selected};
      ++spec_index;
    }
  }

  void read_block(parsed_arguments& parsed) const {
    constexpr bool required = contains_required_block(std::make_index_sequence<arity>{});
    if (rb_block_given_p()) {
      parsed.block_value = value{protect(rb_block_proc)};
    } else if (required) {
      throw ruby_error(make_exception(rb_eArgError, "rbxx: required block was not given"));
    }
  }

  template <std::size_t... Index>
  static consteval bool contains_required_block(std::index_sequence<Index...>) {
    return (is_block_argument_v<std::tuple_element_t<Index, Tuple>> || ...);
  }

  template <std::size_t... Index> static consteval bool has_rest(std::index_sequence<Index...>) {
    return (is_rest_argument_v<std::tuple_element_t<Index, Tuple>> || ...);
  }

  std::vector<argument_spec> specs_;
};

template <typename Argument>
auto load_parsed_argument(const parsed_arguments& parsed, std::size_t index) {
  if constexpr (is_block_argument_v<Argument>) {
    return block{parsed.block_value};
  } else if constexpr (is_optional_block_argument_v<Argument>) {
    return parsed.block_value.is_nil() ? optional_block{} : optional_block{parsed.block_value};
  } else if constexpr (is_rest_argument_v<Argument>) {
    return args{parsed.rest};
  } else {
    return load_argument<Argument>(parsed.slots[index]);
  }
}

} // namespace detail

} // namespace rbxx

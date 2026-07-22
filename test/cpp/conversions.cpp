#include <rbxx/rbxx.hpp>

#include <cstdint>
#include <string>
#include <string_view>

namespace rx = rbxx;

namespace {

template <typename T> VALUE round_trip(VALUE, VALUE input) {
  VALUE pending = Qnil;
  VALUE result = Qnil;
  try {
    result = rx::type_caster<T>::dump(rx::type_caster<T>::load(rx::value{input})).raw();
  } catch (...) {
    pending = rx::detail::translate_current_exception();
  }
  if (!NIL_P(pending)) {
    rb_exc_raise(pending);
  }
  return result;
}

VALUE match_mask(VALUE, VALUE input) {
  rx::value wrapped{input};
  int mask = 0;
  mask |= rx::type_caster<int>::matches(wrapped) ? 1 : 0;
  mask |= rx::type_caster<double>::matches(wrapped) ? 2 : 0;
  mask |= rx::type_caster<std::string>::matches(wrapped) ? 4 : 0;
  mask |= rx::type_caster<bool>::matches(wrapped) ? 8 : 0;
  return INT2NUM(mask);
}

} // namespace

RBXX_EXTENSION(conversions) {
  VALUE root = rx::protect([] { return rb_define_module("RbxxTest"); });
  VALUE conversions = rx::protect([root] { return rb_define_module_under(root, "Conversions"); });
  rx::protect([conversions] {
    rb_define_module_function(conversions, "bool", RUBY_METHOD_FUNC(round_trip<bool>), 1);
    rb_define_module_function(conversions, "int8", RUBY_METHOD_FUNC(round_trip<std::int8_t>), 1);
    rb_define_module_function(conversions, "uint8", RUBY_METHOD_FUNC(round_trip<std::uint8_t>), 1);
    rb_define_module_function(conversions, "int32", RUBY_METHOD_FUNC(round_trip<std::int32_t>), 1);
    rb_define_module_function(conversions, "uint32", RUBY_METHOD_FUNC(round_trip<std::uint32_t>),
                              1);
    rb_define_module_function(conversions, "int64", RUBY_METHOD_FUNC(round_trip<std::int64_t>), 1);
    rb_define_module_function(conversions, "uint64", RUBY_METHOD_FUNC(round_trip<std::uint64_t>),
                              1);
    rb_define_module_function(conversions, "float", RUBY_METHOD_FUNC(round_trip<float>), 1);
    rb_define_module_function(conversions, "double", RUBY_METHOD_FUNC(round_trip<double>), 1);
    rb_define_module_function(conversions, "cstring", RUBY_METHOD_FUNC(round_trip<const char*>), 1);
    rb_define_module_function(conversions, "string", RUBY_METHOD_FUNC(round_trip<std::string>), 1);
    rb_define_module_function(conversions, "string_view",
                              RUBY_METHOD_FUNC(round_trip<std::string_view>), 1);
    rb_define_module_function(conversions, "value", RUBY_METHOD_FUNC(round_trip<rx::value>), 1);
    rb_define_module_function(conversions, "match_mask", RUBY_METHOD_FUNC(match_mask), 1);
  });
}

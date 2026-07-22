#include <rbxx/rbxx.hpp>

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace rx = rbxx;

namespace {

int destructor_count = 0;

struct destruction_guard {
  ~destruction_guard() { ++destructor_count; }
};

VALUE cpp_runtime_error(VALUE) {
  VALUE pending = Qnil;
  try {
    throw std::runtime_error("native failure");
  } catch (...) {
    pending = rx::detail::translate_current_exception();
  }
  rb_exc_raise(pending);
}

template <typename Exception> VALUE mapped_exception(VALUE) {
  VALUE pending = Qnil;
  try {
    throw Exception("mapped failure");
  } catch (...) {
    pending = rx::detail::translate_current_exception();
  }
  rb_exc_raise(pending);
}

VALUE mapped_bad_alloc(VALUE) {
  VALUE pending = Qnil;
  try {
    throw std::bad_alloc{};
  } catch (...) {
    pending = rx::detail::translate_current_exception();
  }
  rb_exc_raise(pending);
}

VALUE mapped_unknown(VALUE) {
  VALUE pending = Qnil;
  try {
    throw 42;
  } catch (...) {
    pending = rx::detail::translate_current_exception();
  }
  rb_exc_raise(pending);
}

VALUE cpp_exception_through_protect(VALUE) {
  VALUE pending = Qnil;
  try {
    rx::protect([]() -> VALUE { throw std::runtime_error("protected native failure"); });
  } catch (...) {
    pending = rx::detail::translate_current_exception();
  }
  rb_exc_raise(pending);
}

VALUE call_with_guard(VALUE, VALUE callable) {
  VALUE pending = Qnil;
  try {
    destruction_guard guard;
    rx::protect([callable] { return rb_funcall(callable, rb_intern("call"), 0); });
  } catch (...) {
    pending = rx::detail::translate_current_exception();
  }
  if (!NIL_P(pending)) {
    rb_exc_raise(pending);
  }
  return Qnil;
}

VALUE call_and_reraise(VALUE, VALUE callable) {
  VALUE pending = Qnil;
  try {
    try {
      rx::protect([callable] { return rb_funcall(callable, rb_intern("call"), 0); });
    } catch (const rx::ruby_error& error) {
      error.reraise();
    }
  } catch (...) {
    pending = rx::detail::translate_current_exception();
  }
  if (!NIL_P(pending)) {
    rb_exc_raise(pending);
  }
  return Qnil;
}

VALUE current_destructor_count(VALUE) { return INT2NUM(destructor_count); }

VALUE object_copy_survives_gc(VALUE) {
  VALUE string = rx::protect(rb_utf8_str_new_cstr, "copy survived");
  rx::object first{string};
  rx::object second = first;
  first.reset();
  rx::protect(rb_gc_start);
  return second.raw();
}

VALUE object_move_survives_gc(VALUE) {
  VALUE string = rx::protect(rb_utf8_str_new_cstr, "move survived");
  rx::object first{string};
  rx::object second = std::move(first);
  rx::protect(rb_gc_start);
  return second.raw();
}

VALUE object_survives_compaction(VALUE) {
  VALUE string = rx::protect(rb_utf8_str_new_cstr, "compaction survived");
  rx::object pinned{string};
  rx::protect([] { return rb_funcall(rb_mGC, rb_intern("compact"), 0); });
  return pinned.raw();
}

VALUE inspect_ruby_error(VALUE, VALUE callable) {
  VALUE pending = Qnil;
  try {
    rx::protect([callable] { return rb_funcall(callable, rb_intern("call"), 0); });
  } catch (const rx::ruby_error& error) {
    std::string klass = error.ruby_class_name();
    std::string message = error.message();
    VALUE result = rx::protect(rb_ary_new_capa, 2L);
    rx::protect(rb_ary_push, result, rx::protect(rb_utf8_str_new, klass.data(), klass.size()));
    rx::protect(rb_ary_push, result, rx::protect(rb_utf8_str_new, message.data(), message.size()));
    return result;
  } catch (...) {
    pending = rx::detail::translate_current_exception();
  }
  if (!NIL_P(pending)) {
    rb_exc_raise(pending);
  }
  return Qnil;
}

static_assert(std::is_trivially_copyable_v<rx::value>);
static_assert(std::is_nothrow_move_constructible_v<rx::object>);

} // namespace

RBXX_EXTENSION(boundary) {
  VALUE root = rx::protect([] { return rb_define_module("RbxxTest"); });
  VALUE boundary = rx::protect([root] { return rb_define_module_under(root, "Boundary"); });
  rx::protect([boundary] {
    rb_define_module_function(boundary, "cpp_runtime_error", RUBY_METHOD_FUNC(cpp_runtime_error),
                              0);
    rb_define_module_function(boundary, "cpp_exception_through_protect",
                              RUBY_METHOD_FUNC(cpp_exception_through_protect), 0);
    rb_define_module_function(boundary, "mapped_invalid_argument",
                              RUBY_METHOD_FUNC(mapped_exception<std::invalid_argument>), 0);
    rb_define_module_function(boundary, "mapped_out_of_range",
                              RUBY_METHOD_FUNC(mapped_exception<std::out_of_range>), 0);
    rb_define_module_function(boundary, "mapped_range_error",
                              RUBY_METHOD_FUNC(mapped_exception<std::range_error>), 0);
    rb_define_module_function(boundary, "mapped_domain_error",
                              RUBY_METHOD_FUNC(mapped_exception<std::domain_error>), 0);
    rb_define_module_function(boundary, "mapped_bad_alloc", RUBY_METHOD_FUNC(mapped_bad_alloc), 0);
    rb_define_module_function(boundary, "mapped_unknown", RUBY_METHOD_FUNC(mapped_unknown), 0);
    rb_define_module_function(boundary, "call_with_guard", RUBY_METHOD_FUNC(call_with_guard), 1);
    rb_define_module_function(boundary, "call_and_reraise", RUBY_METHOD_FUNC(call_and_reraise), 1);
    rb_define_module_function(boundary, "destructor_count",
                              RUBY_METHOD_FUNC(current_destructor_count), 0);
    rb_define_module_function(boundary, "object_copy_survives_gc",
                              RUBY_METHOD_FUNC(object_copy_survives_gc), 0);
    rb_define_module_function(boundary, "object_move_survives_gc",
                              RUBY_METHOD_FUNC(object_move_survives_gc), 0);
    rb_define_module_function(boundary, "object_survives_compaction",
                              RUBY_METHOD_FUNC(object_survives_compaction), 0);
    rb_define_module_function(boundary, "inspect_ruby_error", RUBY_METHOD_FUNC(inspect_ruby_error),
                              1);
  });
}

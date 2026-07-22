#include <rbxx/rbxx.hpp>

#include <type_traits>

extern "C" void Init_smoke() {
  VALUE root = rb_define_module("RbxxTest");
  rb_define_const(root, "SMOKE", Qtrue);
  rb_define_const(root, "VERSION", rb_utf8_str_new_cstr(rbxx::version));
}

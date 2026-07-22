#include <rbxx/rbxx.hpp>

int multiply(int left, int right) { return left * right; }

RBXX_EXTENSION(precompiled_demo) {
  rbxx::define_module("PrecompiledDemo").def("multiply", &multiply);
}

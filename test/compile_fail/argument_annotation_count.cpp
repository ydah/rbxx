// EXPECT: rbxx: argument annotation count must match callable parameters
#include <rbxx/rbxx.hpp>

int add(int left, int right) { return left + right; }

void compile_failure() { rbxx::define_module("Failure").def("add", &add, rbxx::arg("left")); }

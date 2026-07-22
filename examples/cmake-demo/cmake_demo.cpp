#include <rbxx/rbxx.hpp>

int square(int input) { return input * input; }

RBXX_EXTENSION(cmake_demo) {
  rbxx::define_module("CmakeDemo").def("square", &square);
}

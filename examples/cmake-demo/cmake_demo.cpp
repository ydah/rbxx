#include <rbxx/rbxx.hpp>
#include <zlib.h>

#include <string>

int square(int input) { return input * input; }
std::string linked_zlib_version() { return zlibVersion(); }

RBXX_EXTENSION(cmake_demo) {
  rbxx::define_module("CmakeDemo").def("square", &square).def("zlib_version", &linked_zlib_version);
}

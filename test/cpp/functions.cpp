#include <rbxx/rbxx.hpp>

#include <functional>
#include <stdexcept>
#include <string>

namespace rx = rbxx;

namespace {

int last_void_value = 0;

int zero_arguments() { return 42; }
int one_argument(int a) { return a; }
int two_arguments(int a, int b) { return a + b; }
int three_arguments(int a, int b, int c) { return a + b + c; }
int four_arguments(int a, int b, int c, int d) { return a + b + c + d; }
int five_arguments(int a, int b, int c, int d, int e) { return a + b + c + d + e; }

int mutable_reference(int& input) { return ++input; }
std::string constant_reference(const std::string& input) { return input + "!"; }
void set_last_value(int input) { last_void_value = input; }
int read_last_value() { return last_void_value; }
int throws_exception() { throw std::runtime_error("function failure"); }

} // namespace

RBXX_EXTENSION(functions) {
  rx::module functions = rx::define_module("RbxxTest");
  VALUE nested = rx::protect(
      [functions] { return rb_define_module_under(functions.get().raw(), "Functions"); });
  rx::module module{rx::value{nested}};
  module.def("zero", &zero_arguments)
      .def("one", &one_argument)
      .def("two", &two_arguments)
      .def("three", &three_arguments)
      .def("four", &four_arguments)
      .def("five", &five_arguments)
      .def("mutable_reference", &mutable_reference)
      .def("constant_reference", &constant_reference)
      .def("set_last_value", &set_last_value)
      .def("read_last_value", &read_last_value)
      .def("lambda_value", [](int value) { return value * 2; })
      .def("std_function", std::function<int(int)>{[](int value) { return value + 10; }})
      .def("throws_exception", &throws_exception);
  rx::define_global_function("rbxx_global_sum", &two_arguments);
}

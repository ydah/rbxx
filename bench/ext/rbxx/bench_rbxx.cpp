#include <rbxx/rbxx.hpp>

#include <string>
#include <vector>

namespace {

int int_value() { return 42; }
std::string string_roundtrip(std::string input) { return input; }
std::vector<double> vector_roundtrip(std::vector<double> input) { return input; }

class counter {
public:
  explicit counter(int initial) : value_(initial) {}
  [[nodiscard]] int value() const { return value_; }

private:
  int value_;
};

} // namespace

RBXX_EXTENSION(bench_rbxx) {
  rbxx::module module = rbxx::define_module("BenchRbxx");
  module.def("int_value", &int_value)
      .def("string_roundtrip", &string_roundtrip)
      .def("vector_roundtrip", &vector_roundtrip);
  module.def_class<counter>("Counter").def(rbxx::init<int>()).def<&counter::value>("value");
}

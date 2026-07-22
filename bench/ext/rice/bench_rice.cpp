#include <rice/rice.hpp>
#include <rice/stl.hpp>

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

extern "C" void Init_bench_rice() {
  Rice::Module module = Rice::define_module("BenchRice");
  module.define_module_function("int_value", &int_value)
      .define_module_function("string_roundtrip", &string_roundtrip)
      .define_module_function("vector_roundtrip", &vector_roundtrip);
  Rice::define_class_under<counter>(module, "Counter")
      .define_constructor(Rice::Constructor<counter, int>())
      .define_method("value", &counter::value);
}

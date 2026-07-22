#include <rbxx/rbxx.hpp>

namespace {

class counter {
public:
  explicit counter(int initial) : value_(initial) {}
  void add(int delta) { value_ += delta; }
  void subtract(int delta) { value_ -= delta; }
  void reset() { value_ = 0; }
  [[nodiscard]] int value() const { return value_; }
  [[nodiscard]] int doubled() const { return value_ * 2; }

private:
  int value_;
};

} // namespace

RBXX_EXTENSION(class_binding) {
  rbxx::define_module("ClassBinding")
      .def_class<counter>("Counter")
      .def(rbxx::init<int>(), rbxx::kwarg("start") = 0)
      .def("add", &counter::add, rbxx::arg("delta"))
      .def("subtract", &counter::subtract, rbxx::arg("delta"))
      .def("reset", &counter::reset)
      .def<&counter::value>("value")
      .def<&counter::doubled>("doubled");
}

#include <rbxx/rbxx.hpp>

#include <string>

namespace rx = rbxx;

namespace {

int add_default(int left, int right) { return left + right; }

std::string mixed_arguments(const std::string& prefix, int count, bool bang) {
  std::string result;
  for (int index = 0; index < count; ++index) {
    result += prefix;
  }
  return bang ? result + "!" : result;
}

int apply_block(int value, rx::block callback) { return callback.call<int>(value) * 2; }

int apply_optional_block(int value, rx::optional_block callback) {
  return callback ? callback.call<int>(value) : value;
}

int sum_rest(int first, rx::args rest) {
  int result = first;
  for (rx::value item : rest.values()) {
    result += rx::from_ruby<int>(item);
  }
  return result;
}

struct token {
  explicit token(std::string text) : value(std::move(text)) {}
  std::string value;
};

std::string choose_int(int) { return "integer"; }
std::string choose_string(const std::string&) { return "string"; }
std::string choose_token(const token&) { return "token"; }
std::string first_integer(int) { return "first"; }
std::string second_integer(long) { return "second"; }

struct number_box {
  explicit number_box(int initial) : number(initial) {}

  [[nodiscard]] number_box add(int other) const { return number_box{number + other}; }
  [[nodiscard]] int compare(const number_box& other) const {
    return (number > other.number) - (number < other.number);
  }
  [[nodiscard]] int index(int offset) const { return number + offset; }
  [[nodiscard]] int scale(int factor) const { return number * factor; }
  [[nodiscard]] int value() const { return number; }

  int number;
};

} // namespace

RBXX_EXTENSION(arguments) {
  rx::module root = rx::define_module("RbxxTest");
  VALUE nested =
      rx::protect([root] { return rb_define_module_under(root.get().raw(), "Arguments"); });
  rx::module arguments{rx::value{nested}};

  arguments.def("add_default", &add_default, rx::arg("left"), rx::arg("right") = 10)
      .def("mixed", &mixed_arguments, rx::arg("prefix"), rx::kwarg("count"),
           rx::kwarg("bang") = false)
      .def("apply_block", &apply_block, rx::arg("value"))
      .def("apply_optional_block", &apply_optional_block, rx::arg("value"))
      .def("sum_rest", &sum_rest, rx::arg("first"))
      .def("choose", &choose_int)
      .def("choose", &choose_string)
      .def("choose", &choose_token)
      .def("definition_order", &first_integer)
      .def("definition_order", &second_integer);

  arguments.def_class<token>("Token")
      .def(rx::init<std::string>())
      .def_attr_reader("value", &token::value);
  arguments.def_class<number_box>("NumberBox")
      .def(rx::init<int>(), rx::kwarg("start") = 0)
      .def("value", &number_box::value)
      .def("scale", &number_box::scale, rx::kwarg("factor") = 2)
      .def(rx::op::add, &number_box::add)
      .def(rx::op::compare, &number_box::compare)
      .def(rx::op::index, &number_box::index);
}

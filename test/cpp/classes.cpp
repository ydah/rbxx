#include <rbxx/rbxx.hpp>

#include <memory>
#include <string>

namespace rx = rbxx;

namespace {

struct counter {
  static inline int created_count = 0;
  static inline int destroyed_count = 0;

  int number = 0;

  counter() { ++created_count; }
  explicit counter(int initial) : number(initial) { ++created_count; }
  counter(const counter& other) : number(other.number) { ++created_count; }
  ~counter() { ++destroyed_count; }

  [[nodiscard]] int value() const { return number; }
  void add(int delta) { number += delta; }
};

struct base {
  explicit base(int initial) : number(initial) {}
  virtual ~base() = default;
  [[nodiscard]] int base_value() const { return number; }
  int number;
};

struct derived : base {
  explicit derived(int initial) : base(initial) {}
  [[nodiscard]] int doubled() const { return number * 2; }
};

struct child {
  explicit child(int initial) : number(initial) {}
  [[nodiscard]] int value() const { return number; }
  int number;
};

struct owner {
  explicit owner(int initial) : owned_child(initial) {}
  [[nodiscard]] child* get_child() { return &owned_child; }
  child owned_child;
};

struct ruby_holder {
  explicit ruby_holder(rx::value initial) : held(initial) {}
  [[nodiscard]] rx::value get() const { return held.get(); }
  void set(rx::value updated) { held.set(updated); }
  rx::member_value held;
};

struct unregistered {};

int read_base(const base& input) { return input.base_value(); }
void consume_unregistered(unregistered&) {}
std::unique_ptr<child> make_owned_child(int value) { return std::make_unique<child>(value); }
std::shared_ptr<child> make_shared_child(int value) { return std::make_shared<child>(value); }

} // namespace

RBXX_EXTENSION(classes) {
  rx::module root = rx::define_module("RbxxTest");
  VALUE nested_value =
      rx::protect([root] { return rb_define_module_under(root.get().raw(), "Classes"); });
  rx::module classes{rx::value{nested_value}};

  classes.def_class<counter>("Counter")
      .def(rx::init<>())
      .def(rx::init<int>())
      .def("value", &counter::value)
      .def("add", &counter::add)
      .def_attr_accessor("number", &counter::number)
      .def_static("created", [] { return counter::created_count; })
      .def_static("destroyed", [] { return counter::destroyed_count; });

  classes.def_class<base>("Base").def(rx::init<int>()).def("base_value", &base::base_value);
  classes.def_class<derived, base>("Derived")
      .def(rx::init<int>())
      .def("doubled", &derived::doubled);
  classes.def("read_base", &read_base);

  classes.def_class<child>("Child").def(rx::init<int>()).def("value", &child::value);
  classes.def_class<owner>("Owner")
      .def(rx::init<int>())
      .def("child", &owner::get_child, rx::policy::reference);
  classes.def("make_owned_child", &make_owned_child);
  classes.def("make_shared_child", &make_shared_child);

  classes.def_class<ruby_holder>("RubyHolder")
      .def(rx::init<rx::value>())
      .def("value", &ruby_holder::get)
      .def("value=", &ruby_holder::set);

  classes.def("consume_unregistered", &consume_unregistered);
}

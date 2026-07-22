#include <rbxx/rbxx.hpp>

#include <array>
#include <chrono>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

namespace rx = rbxx;

namespace {

template <typename T> T identity(T input) { return input; }

using nested_type = std::vector<std::map<std::string, std::optional<int>>>;
using tuple_type = std::tuple<int, std::string, bool>;
using variant_type = std::variant<int, std::string>;
using milliseconds = std::chrono::milliseconds;

} // namespace

RBXX_EXTENSION(stl) {
  rx::module root = rx::define_module("RbxxTest");
  VALUE nested = rx::protect([root] { return rb_define_module_under(root.get().raw(), "Stl"); });
  rx::module stl{rx::value{nested}};

  stl.def("vector", &identity<std::vector<int>>)
      .def("array", &identity<std::array<int, 3>>)
      .def("map", &identity<std::map<std::string, int>>)
      .def("unordered_map", &identity<std::unordered_map<std::string, int>>)
      .def("set", &identity<std::set<int>>)
      .def("pair", &identity<std::pair<int, std::string>>)
      .def("tuple", &identity<tuple_type>)
      .def("optional", &identity<std::optional<int>>)
      .def("variant", &identity<variant_type>)
      .def("duration", &identity<milliseconds>)
      .def("path", &identity<std::filesystem::path>)
      .def("nested", &identity<nested_type>)
      .def("string", &identity<std::string>);
}

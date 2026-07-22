#include <rbxx/rbxx.hpp>

#include <functional>
#include <vector>

std::vector<int> transform(const std::vector<int>& input, const std::function<int(int)>& callback) {
  std::vector<int> result;
  result.reserve(input.size());
  for (int item : input) {
    result.push_back(callback(item));
  }
  return result;
}

RBXX_EXTENSION(callback) { rbxx::define_module("CallbackExample").def("transform", &transform); }

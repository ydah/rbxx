// EXPECT: rbxx: function argument type has no type_caster
#include <rbxx/rbxx.hpp>

enum class unbound_argument { value };

void consume(unbound_argument) {}

void compile_failure() { rbxx::define_module("Failure").def("consume", &consume); }

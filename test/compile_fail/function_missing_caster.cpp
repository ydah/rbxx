// EXPECT: rbxx: function argument type has no type_caster
#include <rbxx/rbxx.hpp>

struct unbound_argument {};

void consume(unbound_argument) {}

void compile_failure() { rbxx::define_module("Failure").def("consume", &consume); }

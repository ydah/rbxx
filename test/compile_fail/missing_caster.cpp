// EXPECT: rbxx: type has no type_caster
#include <rbxx/rbxx.hpp>

struct unbound_type {};

void compile_failure() { (void)rbxx::to_ruby(unbound_type{}); }

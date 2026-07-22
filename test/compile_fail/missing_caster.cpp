// EXPECT: rbxx: type has no type_caster
#include <rbxx/rbxx.hpp>

enum class unbound_type { value };

void compile_failure() { (void)rbxx::to_ruby(unbound_type::value); }

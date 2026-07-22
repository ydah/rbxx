// EXPECT: rbxx: nogvl arguments must not access Ruby without the GVL
#include <rbxx/rbxx.hpp>

void unsafe(rbxx::value) {}

auto wrapped = rbxx::nogvl(&unsafe);

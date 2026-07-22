# API overview

The generated Doxygen reference is built with `doxygen Doxyfile`. The stable v0.1 entry points are:

- Boundary and values: `rbxx::value`, `rbxx::object`, `rbxx::member_value`, `rbxx::protect`,
  `rbxx::ruby_error`, `rbxx::gc_guard`, `RBXX_EXTENSION`.
- Conversion: `rbxx::type_caster<T>`, `rbxx::to_ruby`, `rbxx::from_ruby`, and `rbxx/stl.hpp`.
- Functions: `rbxx::define_module`, `module::def`, `define_global_function`, `arg`, `kwarg`, `block`,
  `optional_block`, and `args`.
- Classes: `module::def_class`, `class_::def`, `class_::def_static`, attribute helpers,
  `class_::def_iterable`, `init`, operator names, return policies, and `keep_alive`.
- Threads: `rbxx::nogvl`, `rbxx::nogvl_interruptible`, and Proc-to-`std::function` conversion.
- Tooling: `create_rbxx_makefile`, `create_rbxx_cmake_makefile`, `rbxx new`, `rbxx::rbxx` for CMake,
  and `Rbxx::RakeTasks`.

Headers under `rbxx::detail` are implementation details and may change without compatibility notice.

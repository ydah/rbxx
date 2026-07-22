# Class binding

This is the smallest complete TypedData-backed C++ class example.

```sh
cd examples/class-binding
ruby extconf.rb && make
ruby run.rb
```

The compile-time form `def<&counter::value>` gives a zero-argument method the fixed-arity fast
path. Use ordinary `.def("add", &counter::add, ...)` for methods with arguments or overloads.

# Ruby callbacks

A Ruby `Proc` converts to `std::function<R(Args...)>`:

```cpp
int apply(const std::function<int(int)>& fn, int value) { return fn(value); }
module.def("apply", &apply);
```

```ruby
Native.apply(->(value) { value * 2 }, 21) # => 42
```

The Proc is pinned by `rbxx::object`. Arguments and return values use the normal type casters. A Ruby
exception raised by the Proc crosses C++ as `rbxx::ruby_error` and is re-raised as the original Ruby
exception object.

The callback must execute while the current Ruby thread holds the GVL. With `RBXX_DEBUG=1`, calling
it without the GVL raises a clear C++ runtime error before touching Ruby. A callback must not be
captured inside work passed to `rbxx::nogvl`; restructure the native operation so callbacks occur
before releasing or after reacquiring the GVL.

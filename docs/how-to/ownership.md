# Ownership and lifetime

Use values for transfer, smart pointers for ownership, and explicit policies for borrowed results.

| C++ return | Ruby wrapper | Guidance |
|---|---|---|
| `T` | owned moved/copied value | Default for independent results |
| `std::unique_ptr<T>` | owned pointer | Ruby deletes it during TypedData free |
| `std::shared_ptr<T>` | shared owner | Wrapper retains the shared pointer |
| `T*` or `T&` | borrowed pointer | Add an explicit policy and consider `keep_alive` |

For a child stored inside an owner:

```cpp
binding.def("child", &Owner::child, rbxx::policy::reference,
            rbxx::keep_alive<0, 1>());
```

Index `0` is the return value, `1` is `self`, and `2` onward are Ruby arguments. This example stores
`self` in a hidden Array ivar on the returned child. Ruby's GC therefore sees and compacts the
reference normally. Omitting it makes the borrowed child invalid as soon as its owner is collected.

Use `rbxx::object` for a Ruby object retained by arbitrary C++ state and `rbxx::member_value` for a
Ruby value held as a native object's member. A plain `rbxx::value` is a temporary non-owning view.

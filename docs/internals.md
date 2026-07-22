# Internal design: exceptions, GC, and dispatch

## Exception boundary

Ruby raises by `longjmp`; C++ raises by stack unwinding. Letting either mechanism cross the other's
frames is undefined behavior. `rbxx::protect` invokes Ruby through `rb_protect`, clears `errinfo`, and
throws a pinned `rbxx::ruby_error`. The extension and method trampolines catch every C++ exception,
translate it to a Ruby exception VALUE, leave the C++ scope, and only then call `rb_exc_raise`.

Callbacks reuse the same path, which preserves the original Ruby exception object. No destructor is
expected to run after the final `rb_exc_raise` point.

## GC and TypedData

Each bound C++ instance is a TypedData wrapper containing a native pointer, an ownership mode, and an
optional shared owner. Owned wrappers delete the pointer, borrowed wrappers do not, and shared
wrappers retain `std::shared_ptr<T>`. Free and size callbacks are `noexcept` and do not call Ruby.

`rbxx::object` and `rbxx::member_value` use registered-address cells so CRuby marks and updates their
VALUE during compaction. `keep_alive` stores visible Ruby references in a hidden ivar Array, avoiding
an opaque native-only object graph.

## Dispatch

General methods use an `(owner, method ID)` registry and a Ruby arity `-1` trampoline. This supports
defaults, keywords, blocks, overload scoring, policies, and inheritance through one implementation.
Single zero-argument members receive a fixed-arity slot automatically. The compile-time
`def<&T::method>` form bakes the member pointer into a unique trampoline and measured within 1.02x of
hand-written C. Adding another overload redefines the Ruby entry point to the general dispatcher.

Registries are written during extension initialization under the GVL and read after initialization.
They intentionally live for process lifetime, avoiding shutdown-order calls into Ruby.

# Releasing the GVL

Wrap blocking or CPU-heavy pure C++ work with `rbxx::nogvl`:

```cpp
int compress_file(std::string path);
module.def("compress_file", rbxx::nogvl(&compress_file));
```

Ruby arguments are converted to owned C++ values before the adapter releases the GVL. Native work
runs through `rb_thread_call_without_gvl`; its result and any captured C++ exception are handled only
after the GVL has been reacquired. Direct `rbxx::value`, `rbxx::object`, block, args, and callback
types are rejected at compile time.

For cooperative cancellation, provide a `noexcept` unblock function:

```cpp
std::atomic_bool stop = false;
void request_stop() noexcept { stop.store(true); }
int work(int limit); // polls stop

module.def("work", rbxx::nogvl_interruptible(&work, &request_stop));
```

Ruby `Thread#raise` invokes the unblock hook. The worker must stop promptly and must not call Ruby C
APIs from either the worker or unblock function.

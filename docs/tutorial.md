# Tutorial: wrap zlib in 30 minutes

This walkthrough creates a source gem that exposes zlib's version and CRC32 implementation. The
finished extension uses CMake because zlib is an external dependency; use the mkmf generator when
the wrapped C++ code has no separate build system.

## 1. Generate the project

```sh
rbxx new zlib_native --cmake
cd zlib_native
```

The generated gem already has a gemspec, extension task, test task, CI, and native-gem release
workflow. Run its initial test once before changing it:

```sh
bundle install
bundle exec rake test
```

## 2. Link zlib

Add zlib to `ext/zlib_native/CMakeLists.txt`:

```cmake
find_package(rbxx CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
add_library(zlib_native MODULE zlib_native.cpp)
target_link_libraries(zlib_native PRIVATE rbxx::rbxx ZLIB::ZLIB)
set_target_properties(zlib_native PROPERTIES
  PREFIX "" SUFFIX ".${Ruby_DLEXT}"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
  LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}")
```

`rbxx::rbxx` supplies C++20 and the Ruby/rbxx include paths. `ZLIB::ZLIB` supplies zlib's headers
and linker input.

## 3. Bind the C API behind C++ values

Replace the generated source with:

```cpp
#include <rbxx/rbxx.hpp>
#include <zlib.h>
#include <cstdint>
#include <string>

std::string version() { return zlibVersion(); }

std::uint32_t crc32_string(const std::string& input) {
  return static_cast<std::uint32_t>(
      crc32(0, reinterpret_cast<const Bytef*>(input.data()), input.size()));
}

RBXX_EXTENSION(zlib_native) {
  rbxx::define_module("ZlibNative")
      .def("version", &version)
      .def("crc32", &crc32_string, rbxx::arg("string"));
}
```

The extension body is automatically surrounded by the C++→Ruby exception boundary. `std::string`
and `std::uint32_t` use built-in casters.

## 4. Test Ruby behavior

Replace the generated test with:

```ruby
require "minitest/autorun"
require "zlib"
require "zlib_native"

class ZlibNativeTest < Minitest::Test
  def test_crc32
    assert_equal Zlib.crc32("safe boundary"), ZlibNative.crc32("safe boundary")
    refute_empty ZlibNative.version
  end
end
```

Run `bundle exec rake test`. Type errors become Ruby `TypeError`; any Ruby exception raised during
conversion is caught by `rb_protect`, carried through C++ as `rbxx::ruby_error`, and re-raised only
after C++ locals have been destroyed.

## 5. Prepare distribution

Run `bundle exec rake precompiled:dry_run` to inspect the five supported artifacts. The generated
release workflow builds the x86_64 Linux artifact with rake-compiler-dock. Keep the source extension
in `spec.extensions`: it is the fallback for platforms without a matching native gem.

The repository's [CMake zlib example](../examples/cmake-demo) is a directly runnable variant.

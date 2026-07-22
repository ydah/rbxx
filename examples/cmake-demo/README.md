# CMake demo

Build and load the extension from the repository root:

```sh
cmake -S examples/cmake-demo -B tmp/cmake-demo
cmake --build tmp/cmake-demo
ruby -I tmp/cmake-demo -rcmake_demo -e 'p [CmakeDemo.square(9), CmakeDemo.zlib_version]'
```

Installed consumers can set `rbxx_DIR` to the gem's `ext-cmake` directory before calling
`find_package(rbxx CONFIG REQUIRED)`.
This example also uses `find_package(ZLIB)` to demonstrate linking an external C library.

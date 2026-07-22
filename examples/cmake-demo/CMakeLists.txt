cmake_minimum_required(VERSION 3.20)
project(rbxx_cmake_demo LANGUAGES CXX)

list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/../../ext-cmake")
find_package(rbxx CONFIG REQUIRED)
find_package(ZLIB REQUIRED)

add_library(cmake_demo MODULE cmake_demo.cpp)
target_link_libraries(cmake_demo PRIVATE rbxx::rbxx ZLIB::ZLIB)
set_target_properties(cmake_demo PROPERTIES
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED YES
  PREFIX ""
  SUFFIX ".${Ruby_DLEXT}"
)

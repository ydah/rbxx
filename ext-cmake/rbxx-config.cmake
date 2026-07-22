list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")
find_package(Ruby REQUIRED MODULE)

if(NOT TARGET rbxx::rbxx)
  add_library(rbxx::rbxx INTERFACE IMPORTED)
  get_filename_component(_rbxx_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
  set_target_properties(rbxx::rbxx PROPERTIES
    INTERFACE_COMPILE_FEATURES cxx_std_20
    INTERFACE_INCLUDE_DIRECTORIES "${_rbxx_root}/include;${Ruby_INCLUDE_DIRS}"
  )

  if(MSVC)
    target_compile_options(rbxx::rbxx INTERFACE /EHsc /utf-8 /W4)
  else()
    target_compile_options(rbxx::rbxx INTERFACE -Wall -Wextra -fvisibility=hidden)
    if(APPLE)
      target_link_options(rbxx::rbxx INTERFACE -undefined dynamic_lookup)
    endif()
  endif()
  if(WIN32)
    target_link_libraries(rbxx::rbxx INTERFACE "${Ruby_LIBRARY}")
  endif()
endif()

set(rbxx_FOUND TRUE)

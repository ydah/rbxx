# Find CRuby through the selected interpreter's RbConfig.

if(NOT Ruby_EXECUTABLE)
  find_program(Ruby_EXECUTABLE NAMES ruby REQUIRED)
endif()

execute_process(
  COMMAND "${Ruby_EXECUTABLE}" -rrbconfig -e
          "puts %w[rubyhdrdir rubyarchhdrdir libdir LIBRUBY_SO DLEXT].map { |key| RbConfig::CONFIG[key] }"
  RESULT_VARIABLE _ruby_config_status
  OUTPUT_VARIABLE _ruby_config
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT _ruby_config_status EQUAL 0)
  message(FATAL_ERROR "RbConfig failed for ${Ruby_EXECUTABLE}")
endif()

string(REPLACE "\n" ";" _ruby_values "${_ruby_config}")
list(LENGTH _ruby_values _ruby_value_count)
if(_ruby_value_count LESS 5)
  message(FATAL_ERROR "RbConfig returned incomplete extension build settings")
endif()

list(GET _ruby_values 0 Ruby_INCLUDE_DIR)
list(GET _ruby_values 1 Ruby_ARCH_INCLUDE_DIR)
list(GET _ruby_values 2 Ruby_LIBRARY_DIR)
list(GET _ruby_values 3 Ruby_LIBRARY_NAME)
list(GET _ruby_values 4 Ruby_DLEXT)
set(Ruby_INCLUDE_DIRS "${Ruby_INCLUDE_DIR};${Ruby_ARCH_INCLUDE_DIR}")

if(WIN32)
  find_library(Ruby_LIBRARY NAMES "${Ruby_LIBRARY_NAME}" PATHS "${Ruby_LIBRARY_DIR}" REQUIRED)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  Ruby
  REQUIRED_VARS Ruby_EXECUTABLE Ruby_INCLUDE_DIR Ruby_ARCH_INCLUDE_DIR Ruby_DLEXT
)

mark_as_advanced(Ruby_INCLUDE_DIR Ruby_ARCH_INCLUDE_DIR Ruby_LIBRARY_DIR Ruby_LIBRARY_NAME)

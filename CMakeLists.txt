cmake_minimum_required(VERSION 3.20)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" WVP_VERSION_DEFAULT LIMIT_COUNT 1)
set(WVP_VERSION "${WVP_VERSION_DEFAULT}" CACHE STRING "Plugin version (CFBundleShortVersionString)")

set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS version")

project(aegp_webview_panel VERSION ${WVP_VERSION} LANGUAGES CXX)

option(WVP_BUILD_PLUGIN "Build the After Effects plugin bundle (macOS only; needs AE_SDK_ROOT)" ${APPLE})
option(WVP_BUILD_TESTS "Build the unit tests of the platform-neutral code" ON)
option(WVP_DEBUG "Debug build: Web Inspector on the panel and dev-url.txt (never ship it)" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# One strict warning set for every target. Warnings are errors.
set(WVP_WARNING_FLAGS
  -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wold-style-cast
  -Woverloaded-virtual -Wnull-dereference -Wdouble-promotion -Wformat=2 -Wimplicit-fallthrough -Wcast-align)

function(wvp_apply_flags target)
  target_compile_options(${target} PRIVATE ${WVP_WARNING_FLAGS})
endfunction()

# Platform-neutral code: no SDK, no OS API beyond the C++ standard library.
add_library(wvp_core STATIC
  src/core/wvp_json.cpp
  src/core/wvp_queue.cpp
  src/core/wvp_scripts.cpp
  src/core/wvp_theme.cpp
  src/core/wvp_trust.cpp
  src/core/wvp_utf.cpp)
target_include_directories(wvp_core PUBLIC src/core)
wvp_apply_flags(wvp_core)

if(WVP_BUILD_TESTS)
  enable_testing()
  add_executable(wvp_tests tests/unit/test_main.cpp src/mac/wvp_keys.cpp)
  target_include_directories(wvp_tests PRIVATE src/mac)
  target_link_libraries(wvp_tests PRIVATE wvp_core)
  wvp_apply_flags(wvp_tests)
  add_test(NAME wvp_tests COMMAND wvp_tests)
endif()

if(WVP_BUILD_PLUGIN)
  if(NOT APPLE)
    message(FATAL_ERROR "The plugin is macOS only for now; src/win/README.md describes the Windows plan.")
  endif()
  include(cmake/plugin_mac.cmake)
endif()

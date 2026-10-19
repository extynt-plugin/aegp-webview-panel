# The macOS plugin bundle, laid out like the SDK samples (.plugin, package type AEgx). The SDK is only referenced
# through AE_SDK_ROOT; nothing from it is copied into this repository.

if(NOT AE_SDK_ROOT AND DEFINED ENV{AE_SDK_ROOT})
  set(AE_SDK_ROOT "$ENV{AE_SDK_ROOT}")
endif()
if(NOT AE_SDK_ROOT)
  message(FATAL_ERROR "AE_SDK_ROOT is not set. Download the After Effects SDK from Adobe's developer site, unpack it, "
                      "and pass -DAE_SDK_ROOT=/path/to/AfterEffectsSDK (see README.md).")
endif()
if(EXISTS "${AE_SDK_ROOT}/Examples/Headers/AE_GeneralPlug.h")
  set(WVP_SDK_EXAMPLES "${AE_SDK_ROOT}/Examples")
elseif(EXISTS "${AE_SDK_ROOT}/Headers/AE_GeneralPlug.h")
  set(WVP_SDK_EXAMPLES "${AE_SDK_ROOT}")
else()
  message(FATAL_ERROR "AE_SDK_ROOT=${AE_SDK_ROOT} does not contain Examples/Headers/AE_GeneralPlug.h")
endif()
message(STATUS "After Effects SDK headers: ${WVP_SDK_EXAMPLES}/Headers")

enable_language(OBJCXX)
set(CMAKE_OBJCXX_STANDARD 17)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Info.plist.in" "${CMAKE_BINARY_DIR}/Info.plist" @ONLY)

add_library(wvpanel MODULE
  src/ae/wvp_entry.cpp
  src/ae/wvp_host.cpp
  src/ae/wvp_ops.cpp
  src/ae/wvp_suites.cpp
  src/mac/wvp_bridge.mm
  src/mac/wvp_install.mm
  src/mac/wvp_keys.cpp
  src/ae/wvp_panel_ae.mm
  src/mac/wvp_log_mac.mm
  src/mac/wvp_monitor.mm
  src/mac/wvp_panel.mm
  src/mac/wvp_panel_delegates.mm
  src/mac/wvp_webview.mm)

target_include_directories(wvpanel PRIVATE src src/ae src/core src/mac)
target_include_directories(wvpanel SYSTEM PRIVATE
  "${WVP_SDK_EXAMPLES}/Headers" "${WVP_SDK_EXAMPLES}/Headers/SP" "${WVP_SDK_EXAMPLES}/Resources")
target_link_libraries(wvpanel PRIVATE wvp_core "-framework CoreFoundation" "-framework Cocoa" "-framework WebKit")
target_compile_definitions(wvpanel PRIVATE WVP_VERSION="${WVP_VERSION}" WVP_DEBUG=$<BOOL:${WVP_DEBUG}>)
wvp_apply_flags(wvpanel)
target_compile_options(wvpanel PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)
target_compile_options(wvpanel PRIVATE $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc -Wobjc-missing-super-calls>)
target_link_options(wvpanel PRIVATE -Wl,-dead_strip)

set_target_properties(wvpanel PROPERTIES
  BUNDLE TRUE
  BUNDLE_EXTENSION "plugin"
  PREFIX ""
  OUTPUT_NAME "wvpanel"
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_BINARY_DIR}/Info.plist")

set(WVP_RESOURCES "$<TARGET_BUNDLE_CONTENT_DIR:wvpanel>/Resources")
add_custom_command(TARGET wvpanel POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "${WVP_RESOURCES}"
  COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/scripts/rez-pipl.sh" "${WVP_SDK_EXAMPLES}"
          "${CMAKE_CURRENT_SOURCE_DIR}/resources/wvpanel_PiPL.r" "${WVP_RESOURCES}/wvpanel.rsrc"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/web" "${WVP_RESOURCES}/web"
  COMMENT "Compiling the PiPL with Rez and copying the web page into the bundle"
  VERBATIM)

cmake_minimum_required(VERSION 3.31)

# Populate our version variables
include(cmake/get_version_from_git.cmake)
get_version_from_git()

set (CSP_VERSION_STRING "${CSP_VERSION}")

project(CSP
    DESCRIPTION "Connected Spaces Platform" VERSION ${CSP_VERSION_STRING}
    LANGUAGES C CXX)

# ------------------------------------------------------------------------------------------------------------------
# First populate variables with our files
set(CSP_ROOT        ${CMAKE_CURRENT_SOURCE_DIR}/Library)
set(CSP_INCLUDE_DIR ${CSP_ROOT}/include)
set(CSP_SOURCE_DIR  ${CSP_ROOT}/src)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/common_files.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/core_files.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/multiplayer_files.cmake)

set(CSP_PUBLIC_SOURCES
  ${CSP_COMMON_PUBLIC_INCLUDES}
  ${CSP_CORE_PUBLIC_INCLUDES}
  ${CSP_MULTIPLAYER_PUBLIC_INCLUDES}
)

set(CSP_PRIVATE_SOURCES
  ${CSP_COMMON_SOURCES}
  ${CSP_COMMON_PRIVATE_INCLUDES}
  ${CSP_CORE_SOURCES}
  ${CSP_CORE_PRIVATE_INCLUDES}
  ${CSP_MULTIPLAYER_SOURCES}
  ${CSP_MULTIPLAYER_PRIVATE_INCLUDES}
)

# Preserve folder structure for files in IDE
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
  ${CSP_PUBLIC_SOURCES}
  ${CSP_PRIVATE_SOURCES}
)

# ------------------------------------------------------------------------------------------------------------------
# Find dependencies
find_package(RapidJSON REQUIRED)
find_package(fmt REQUIRED)
find_package(Async++ REQUIRED)
find_package(tinyspline REQUIRED)
find_package(glm REQUIRED)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  find_package(Poco REQUIRED)
endif()

# OpenUSD. Comes from the openusd Conan package, which wraps a tree built by
# USD's own build_usd.py; pxr_DIR/TBB_DIR are set for us by conanfile.py.
# We use USD's own config rather than a Conan-generated one because its imported
# targets wrap each archive in -Wl,--whole-archive, which USD needs in order to
# keep the static initialisers that register its schemas and plugins.
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  find_package(pxr CONFIG REQUIRED)

  # USD's plugin resources (plugInfo.json plus the per-library resources/
  # directories holding generatedSchema.usda) are runtime data, not link inputs.
  # Natively USD locates them relative to its own shared libraries; under wasm
  # there is no such location, so whatever produces the final module has to
  # place them in the virtual filesystem and point PXR_PLUGINPATH_NAME at them.
  # Derive the path from the found package rather than from Conan, so this keeps
  # working however USD was located.
  get_filename_component(CSP_USD_PREFIX "${PXR_INCLUDE_DIRS}" DIRECTORY)
  set(CSP_USD_PLUGIN_RESOURCES "${CSP_USD_PREFIX}/lib/usd")

  # Where those resources are mounted in the module's virtual filesystem. Not a
  # free choice: USD's own embed rules use /usd, and its plugInfo.json paths are
  # relative to it, so anything reading them has to agree.
  set(CSP_USD_PLUGIN_PATH "/usd")

  if(NOT EXISTS "${CSP_USD_PLUGIN_RESOURCES}/plugInfo.json")
    message(FATAL_ERROR
      "Found USD at '${CSP_USD_PREFIX}' but its plugin resources are missing "
      "('${CSP_USD_PLUGIN_RESOURCES}/plugInfo.json'). USD would link and then "
      "fail on the first UsdStage call. See recipes/openusd/README.md.")
  endif()

  # USD's imported targets each wrap themselves in
  # -Wl,--whole-archive ... -Wl,--no-whole-archive. CMake de-duplicates those
  # repeated flags, collapsing all of them into a single *empty* pair, so every
  # USD archive ends up linked normally. That is not a cosmetic problem: USD
  # registers its schemas, file formats and asset resolvers through static
  # initialisers that nothing references directly, so the linker discards them.
  # The result links cleanly and then fails at runtime with errors like
  # "Failed to manufacture asset resolver Sdf_UsdzResolver from plugin sdf" and
  # "Failed to create primSpec", because plugInfo.json still advertises types
  # whose C++ registration never made it into the module.
  #
  # CMake's LINK_LIBRARY:WHOLE_ARCHIVE feature cannot express this: USD's
  # imported targets reference each other plainly in their own
  # INTERFACE_LINK_LIBRARIES, and CMake refuses to link an item both with a
  # feature and without one ("Impossible to link target ... the link item 'arch'
  # ... has already occurred with the feature 'WHOLE_ARCHIVE'").
  #
  # So build one explicit group by archive path and pass it as a link *option*,
  # which bypasses CMake's link-item bookkeeping entirely. A single pair also
  # survives flag de-duplication, where USD's 33 individual pairs did not.
  #
  # Covers every USD library rather than just the ones CSP calls into: the
  # plugInfo.json USD embeds advertises all of them, and PlugRegistry will try to
  # manufacture types from any of them, so a subset leaves those lookups failing.
  # All of USD therefore lands in the final module; trimming it later means
  # narrowing the library set and the embedded plugInfo together.
  set(CSP_USD_WHOLE_ARCHIVE_GROUP "-Wl,--whole-archive")

  foreach(_csp_usd_lib IN LISTS PXR_LIBRARIES)
    if(NOT PXR_${_csp_usd_lib}_LIBRARY)
      message(FATAL_ERROR "pxrConfig.cmake reported no location for USD library '${_csp_usd_lib}'.")
    endif()

    string(APPEND CSP_USD_WHOLE_ARCHIVE_GROUP " ${PXR_${_csp_usd_lib}_LIBRARY}")
  endforeach()

  string(APPEND CSP_USD_WHOLE_ARCHIVE_GROUP " -Wl,--no-whole-archive")

  # SHELL: keeps CMake from splitting or de-duplicating the group.
  set(CSP_USD_LINK_OPTION "SHELL:${CSP_USD_WHOLE_ARCHIVE_GROUP}")

  # USD carries one --embed-file per resource file in the INTERFACE_LINK_OPTIONS
  # of each of its 33 imported targets - roughly a hundred arguments, each with an
  # absolute path into the Conan cache. Linking all of USD that way overflows the
  # Windows command-line limit outright ("The command line is too long").
  #
  # Replace them with a single directory-level embed, which yields the identical
  # /usd layout. Clearing INTERFACE_LINK_OPTIONS is safe: --embed-file entries are
  # the only thing USD puts there.
  foreach(_csp_usd_lib IN LISTS PXR_LIBRARIES)
    set_property(TARGET ${_csp_usd_lib} PROPERTY INTERFACE_LINK_OPTIONS "")
  endforeach()

  # --embed-file bakes the resources into the .wasm, keeping the module
  # self-contained. A browser-facing build may prefer --preload-file, which moves
  # them into a separate .data file that can be cached independently.
  set(CSP_USD_EMBED_OPTION "SHELL:--embed-file ${CSP_USD_PLUGIN_RESOURCES}@${CSP_USD_PLUGIN_PATH}")
endif()

# Vendored libs
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/signalrclient)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/quickjs)

include(FetchContent)

# Fetch csp serivces
FetchContent_Declare(
  csp-services
  GIT_REPOSITORY https://github.com/magnopus-opensource/csp-services.git
  GIT_TAG        v2.0-d20260421.r0e1d359b9.Release
  GIT_SHALLOW    TRUE
  GIT_PROGRESS   TRUE
)

FetchContent_MakeAvailable(csp-services)

# We need to include the services files in the csp project instead of making a seperate project.
# This is due to both csp-lib and csp-services depending on each other.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/services_files.cmake)

# Preserve folder structure for files in IDE
source_group(TREE ${CSP_SERVICES_DIR} FILES
  ${CSP_SERVICES_SOURCES}
)

# ------------------------------------------------------------------------------------------------------------------
# Create csp-lib

option(CSP_BUILD_SHARED "Build csp as a shared library" OFF)

if(CSP_BUILD_SHARED)
  set(CSP_LIB_TYPE SHARED)
else()
  set(CSP_LIB_TYPE STATIC)
endif()

add_library(csp-lib ${CSP_LIB_TYPE})

target_sources(csp-lib
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS ${CSP_INCLUDE_DIR}
    FILES 
      ${CSP_PUBLIC_SOURCES}
  PRIVATE
    ${CSP_PRIVATE_SOURCES}
    ${CSP_SERVICES_SOURCES}
)

# Set include directories
target_include_directories(csp-lib
  PUBLIC
    # Include dir when building from source
    $<BUILD_INTERFACE:${CSP_INCLUDE_DIR}>
    # Include dir after installation 
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CSP_SOURCE_DIR}
)

# The generated services should be included as a system header for now,
# as these create warnings which manifest as errors due to /wd4100.
target_include_directories(csp-lib
  SYSTEM PRIVATE
    ${CSP_SERVICES_DIR}
)

target_link_libraries(csp-lib
  PRIVATE
    rapidjson
    fmt::fmt
    Async++
    csp-signalr
    tinyspline::tinyspline
    glm::glm
    csp-quickjs
)

if(ANDROID)
  # Link Android's native log library for __android_log_print
  target_link_libraries(csp-lib PRIVATE log)
endif()

# Only include POCO for non-Emscripten builds
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  target_link_libraries(csp-lib
    PRIVATE
      Poco::Poco
      Poco::Net
  )
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  # PRIVATE, as USD does not appear in CSP's public headers. csp-lib is static
  # for wasm, so CMake still records these on the exported target as
  # $<LINK_ONLY:...> and the downstream bindings link resolves them - which is
  # why CSPConfig.cmake.in has to find_dependency(pxr). Same arrangement as
  # tinyspline/glm/msgpack.
  #
  # Linked plainly for their usage requirements - include dirs, PXR_STATIC, and
  # the ~33 --embed-file entries USD carries in INTERFACE_LINK_OPTIONS to mount
  # its plugin resources at /usd. The archives are additionally forced in whole
  # by CSP_USD_LINK_OPTION below; the plain references here resolve nothing extra
  # afterwards, since every symbol is already defined by then.
  target_link_libraries(csp-lib
    PRIVATE
      ${PXR_LIBRARIES}
  )

  # PUBLIC: both of these are needed at the final link of whatever consumes CSP,
  # so they have to propagate through the export to the downstream bindings module.
  target_link_options(csp-lib PUBLIC
    "${CSP_USD_LINK_OPTION}"
    "${CSP_USD_EMBED_OPTION}"
  )

  # USD is built against Emscripten's zlib port rather than a vendored libz, and
  # nothing in its install tree provides those symbols, so the flag has to reach
  # the final link of anything that pulls USD in. PUBLIC so it propagates
  # through the export to the downstream bindings module.
  target_link_options(csp-lib PUBLIC --use-port=zlib)
endif()

set(TINYSPLINE_BUILD_TESTS OFF)
set(TINYSPLINE_BUILD_EXAMPLES OFF)
set(TINYSPLINE_BUILD_DOCS OFF)

# Public as it is a requiremnt that consumers also be >= c++17
target_compile_features(csp-lib PUBLIC cxx_std_17)

target_compile_definitions(csp-lib
  PRIVATE
    # Setup dll export macros if we're building as shared
    $<$<BOOL:${CSP_BUILD_SHARED}>:BUILD_CSP_DLL>
    # Platform macros
    $<$<PLATFORM_ID:Windows>:CSP_WINDOWS>
    $<$<PLATFORM_ID:Darwin>:CSP_MACOSX>
    $<$<PLATFORM_ID:Emscripten>:CSP_WASM>

    # We consume libasync statically
    LIBASYNC_STATIC
    # We have our own default scheduler to remove the need to additional params
    LIBASYNC_CUSTOM_DEFAULT_SCHEDULER

    # Remove codecvt deprecation warnings for msvc
    $<$<CXX_COMPILER_ID:MSVC>:_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING>
    # Disable unused POCO features
    $<$<NOT:$<CXX_COMPILER_ID:Emscripten>>:POCO_NO_INOTIFY>
    $<$<NOT:$<CXX_COMPILER_ID:Emscripten>>:POCO_NO_FILECHANNEL>
    $<$<NOT:$<CXX_COMPILER_ID:Emscripten>>:POCO_NO_SPLITTERCHANNEL>
    $<$<NOT:$<CXX_COMPILER_ID:Emscripten>>:POCO_NO_SYSLOGCHANNEL>
    $<$<NOT:$<CXX_COMPILER_ID:Emscripten>>:POCO_UTIL_NO_INIFILECONFIGURATION>
    $<$<NOT:$<CXX_COMPILER_ID:Emscripten>>:POCO_UTIL_NO_JSONCONFIGURATION>
    $<$<NOT:$<CXX_COMPILER_ID:Emscripten>>:POCO_UTIL_NO_XMLCONFIGURATION>

    # Android + MacOS + iOS
    $<$<OR:$<PLATFORM_ID:Android>,$<PLATFORM_ID:Darwin>>:POCO_NO_WINDOWS_H>
    $<$<OR:$<PLATFORM_ID:Android>,$<PLATFORM_ID:Darwin>>:POCO_NO_FPENVIRONMENT>
    $<$<OR:$<PLATFORM_ID:Android>,$<PLATFORM_ID:Darwin>>:POCO_NO_WSTRING>
    $<$<OR:$<PLATFORM_ID:Android>,$<PLATFORM_ID:Darwin>>:POCO_NO_SHAREDMEMORY>
)

# Warnings as errors flag
# Sets WX/Werror
set_target_properties(csp-lib PROPERTIES
  COMPILE_WARNING_AS_ERROR ON
  OUTPUT_NAME ConnectedSpacesPlatform
  DEBUG_POSTFIX _D
)

# CSP and its internal static libraries may be linked into a shared library,
# so build library targets with PIC on non-Windows platforms.
if(NOT WIN32)
  set_target_properties(
    csp-lib
      PROPERTIES
        POSITION_INDEPENDENT_CODE ON
  )
endif()

target_compile_options(csp-lib
  PRIVATE
    # Needed to compile EntityScriptBindings.cpp
    $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
    # strict standard compliance
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->

    # Set warning level and exceptions for msvc
    $<$<CXX_COMPILER_ID:MSVC>:
      /EHsc     # C++ exception handling (not added by default with Ninja generator)
      /W4       # Sets warning level 4 (high)
      /utf-8    # fmt asked us for this, seems like we need to explicitly opt into unicode on windows
      /wd4251   # Allow stl members to be exported
      /wd4996   # Function maybe unsafe warnings
      /wd4200   # nonstandard extension: zero-sized array
    >

    $<$<CXX_COMPILER_ID:AppleClang,Clang>:
	    -Wextra
	    # --Don't error on deprecation warnings, this is because we use Uri a lot in our services generated code,
	    # which has deprecation warnings for some unused but still generated endpoints.
	    -Wno-error=deprecated-declarations
	    # Don't warn against doing stuff like `return {0}`, which we do in the interop output.
	    -Wno-braced-scalar-init
	    # Don't warn against missing field initializers, e.g. uninitialized fields in structs (because of the wrapper generator)
	    -Wno-missing-field-initializers
	    # Don't error on ignored qualifiers, such as returning const objects. These should be simple to remove at some point.
	    -Wno-ignored-qualifiers
    >

    $<$<CXX_COMPILER_ID:GNU>:
      -Werror
      -Wextra
      # Don't error when naming members the same as class
      -Wno-error=changes-meaning
      -Wno-error=deprecated-declarations
      -Wno-missing-field-initializers
      -Wno-ignored-qualifiers

      # Needed to compile with the Linux Unreal Toolchain
      -Wno-error=attributes

      -fpermissive
    >

    $<$<PLATFORM_ID:Emscripten>:-pthread;-fwasm-exceptions>
)

target_link_options(csp-lib PRIVATE
  $<$<PLATFORM_ID:Emscripten>:
    -pthread
    -fwasm-exceptions
    -sFETCH
  >
)

# Generate version header
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/version.h.in
    # When we remove support for the old build system, we should probably move this into the binary dir
    ${CSP_INCLUDE_DIR}/CSP/version.h
)

# Temporary define until we remove the legacy wrapper gen
if(USE_LEGACY_WRAPPER_GEN)
    target_compile_definitions(csp-lib
        PRIVATE
            USE_LEGACY_WRAPPER_GEN=1
    )
endif()

# Legacy Emscripten wrapper ---------------------------------------

  option(CSP_BUILD_LEGACY_EMSCRIPTEN "Build legacy emscripten project" OFF)

  if(CSP_BUILD_LEGACY_EMSCRIPTEN AND EMSCRIPTEN)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/LegacyEmscriptenWrapper)
  endif()

# OpenUSD runtime smoke test --------------------------------------

  option(CSP_BUILD_USD_SMOKE "Build the OpenUSD wasm runtime smoke test" OFF)

  if(CSP_BUILD_USD_SMOKE)
    # CMAKE_SYSTEM_NAME rather than EMSCRIPTEN: Conan invokes emcc directly
    # instead of going through Emscripten's CMake toolchain file, so the
    # EMSCRIPTEN variable is never set in this build. The rest of this file
    # tests CMAKE_SYSTEM_NAME for the same reason.
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
      message(FATAL_ERROR "CSP_BUILD_USD_SMOKE is Emscripten-only; OpenUSD is currently only wired up for wasm.")
    endif()

    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Tools/UsdSmoke)
  endif()

# Install ---------------------------------------------------------

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Optional local alias for build-tree use
add_library(CSP::csp-lib ALIAS csp-lib)

# Where package config files go
set(CSP_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/CSP")

install(TARGETS csp-lib
  EXPORT CSPTargets
  FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CSPConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/CSPConfig.cmake
  INSTALL_DESTINATION ${CSP_CONFIG_INSTALL_DIR}
  PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/CSPConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/CSPConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/CSPConfigVersion.cmake
  DESTINATION ${CSP_CONFIG_INSTALL_DIR}
)

install(EXPORT CSPTargets
  FILE CSPTargets.cmake
  NAMESPACE CSP::
  DESTINATION ${CSP_CONFIG_INSTALL_DIR}
)

# ------------------------------------------------------------------------------------------------------------------
# Testing
include(CTest)

if(BUILD_TESTING)
  
  find_package(GTest CONFIG REQUIRED)
  find_package(sole REQUIRED)

  # Fetch tiny-process lib
  FetchContent_Declare(
    tiny-process
    GIT_REPOSITORY https://gitlab.com/eidheim/tiny-process-library.git
    GIT_TAG        8bbb5a211c5c9df8ee69301da9d22fb977b27dc1
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
  )

  FetchContent_MakeAvailable(tiny-process)

  add_executable(csp-tests)

  include(${CMAKE_CURRENT_SOURCE_DIR}/Tests/cmake/test_files.cmake)

  target_sources(csp-tests PRIVATE ${CSP_TESTS_SOURCES})

  # Preserve folder structure for files in IDE
  source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
    ${CSP_TESTS_SOURCES}
  )

  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/MultiplayerTestRunner)

  target_link_libraries(csp-tests
    PRIVATE 
      csp-lib
      GTest::gtest
      GTest::gtest_main
      GTest::gmock
      sole::sole
      tiny-process-library::tiny-process-library

      # We need to link a lot of our third party libs here, as well, 
      # becase we either test these directly, or expose them through private headers.
      # This way we don't have to make them public libs for regular consumers.

      # Our tests directly use these libs
      csp-quickjs
      rapidjson
      glm::glm
      fmt::fmt
      Async++
      csp-signalr
  )

  # Only include POCO for non-Emscripten builds
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    target_link_libraries(csp-tests
      PRIVATE
        # Libs exposed through private includes
        # We should find a way of hiding these from headers ideally
        Poco::Poco
        Poco::Net
    )
  endif()

  # Set include directories
  target_include_directories(csp-tests
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/Tests/src
      ${CMAKE_CURRENT_SOURCE_DIR}/MultiplayerTestRunner/include
      # We need to access private includes in our tests
      ${CSP_SOURCE_DIR}
      ${CSP_SERVICES_DIR}
  )

  target_compile_features(csp-tests PRIVATE cxx_std_17)

  target_compile_definitions(csp-tests
    PRIVATE
        CSP_TESTS
        # Create a macro for our test runner output path to run the process.
        # This ensures we have the correct path on all configurations
        # and makes it simpler than copying the executable like we were doing previously.
        MULTIPLAYER_TEST_RUNNER_PATH="$<TARGET_FILE:MultiplayerTestRunner>"
  )

  target_compile_options(csp-tests PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/permissive->

    $<$<CXX_COMPILER_ID:GNU>:
      -Werror
      -Wextra
      -Wno-error=changes-meaning
      -Wno-error=deprecated-declarations
      # uninitialized and maybe-uninitialized are needed our test project.
      # This to avoid treating GCC uninitialized warnings from GoogleTest headers as errors in Linux release builds.
      -Wno-error=uninitialized
      -Wno-error=maybe-uninitialized
      -Wno-missing-field-initializers
      -Wno-ignored-qualifiers
      -fpermissive
    >
  )

  # Ensure csp and MultiplayerTestRunner are built first
  add_dependencies(csp-tests
    csp-lib
    MultiplayerTestRunner
  )

  # Copy out assets folder next to the test executable
  add_custom_command(TARGET csp-tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Tests/assets"
            "$<TARGET_FILE_DIR:csp-tests>/assets"
    COMMENT "Copying assets to output folder"
)

endif()

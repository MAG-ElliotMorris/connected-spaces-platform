#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Async++" for configuration "MinSizeRel"
set_property(TARGET Async++ APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(Async++ PROPERTIES
  IMPORTED_IMPLIB_MINSIZEREL "${_IMPORT_PREFIX}/lib/async++.lib"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/bin/async++.dll"
  )

list(APPEND _cmake_import_check_targets Async++ )
list(APPEND _cmake_import_check_files_for_Async++ "${_IMPORT_PREFIX}/lib/async++.lib" "${_IMPORT_PREFIX}/bin/async++.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)

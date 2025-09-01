# Set the root directory of the FBX SDK
find_path(FBXSDK_INCLUDE_DIR
  NAMES fbxsdk.h
  HINTS "${CMAKE_SOURCE_DIR}/ThirdParty/fbxsdk/include"
  NO_DEFAULT_PATH  # Force the search only in the specified path to avoid conflicts
)

# Search for library files (.lib and.dll) under different build configurations
# Note: The path needs to be explicitly set for each configuration (Debug/Release, etc.)
set(FBX_LIB_DEBUG "${CMAKE_SOURCE_DIR}/ThirdParty/fbxsdk/lib/libfbxsdk.lib")
set(FBX_LIB_RELEASE "${CMAKE_SOURCE_DIR}/ThirdParty/fbxsdk/lib/libfbxsdk.lib")

set(FBX_DLL_DEBUG "${CMAKE_SOURCE_DIR}/ThirdParty/fbxsdk/bin/libfbxsdk.dll")
set(FBX_DLL_RELEASE "${CMAKE_SOURCE_DIR}/ThirdParty/fbxsdk/bin/libfbxsdk.dll")

set(FBX_LIB_MINSIZEREL "${CMAKE_SOURCE_DIR}/ThirdParty/fbxsdk/lib/libfbxsdk.lib")
set(FBX_DLL_MINSIZEREL "${CMAKE_SOURCE_DIR}/ThirdParty/fbxsdk/bin/libfbxsdk.dll")

set(FBX_LIB_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/ThirdParty/fbxsdk/lib/libfbxsdk.lib")
set(FBX_DLL_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/ThirdParty/fbxsdk/bin/libfbxsdk.dll")


# Check whether the required files have been found
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FBXSDK
  REQUIRED_VARS FBXSDK_INCLUDE_DIR FBX_LIB_DEBUG FBX_LIB_RELEASE FBX_DLL_DEBUG FBX_DLL_RELEASE
)

# If found, create the IMPORTED target and set the path with different configurations
if(FBXSDK_FOUND)
  add_library(FBXSDK::FBXSDK SHARED IMPORTED GLOBAL)  # Create a shared library target

  # Settings include directories (universal for all configurations)
  set_target_properties(FBXSDK::FBXSDK PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${FBXSDK_INCLUDE_DIR}"
  )

  # Set the paths of.lib and.dll for the Debug configuration
  set_target_properties(FBXSDK::FBXSDK PROPERTIES
    IMPORTED_IMPLIB_DEBUG "${FBX_LIB_DEBUG}"
    IMPORTED_LOCATION_DEBUG "${FBX_DLL_DEBUG}"
  )

  # Set the paths of.lib and.dll for the Release configuration
  set_target_properties(FBXSDK::FBXSDK PROPERTIES
    IMPORTED_IMPLIB_RELEASE "${FBX_LIB_RELEASE}"
    IMPORTED_LOCATION_RELEASE "${FBX_DLL_RELEASE}"
  )

  # Set the target attributes
  set_target_properties(FBXSDK::FBXSDK PROPERTIES
    IMPORTED_IMPLIB_MINSIZEREL "${FBX_LIB_MINSIZEREL}"
    IMPORTED_LOCATION_MINSIZEREL "${FBX_DLL_MINSIZEREL}"
  )

  set_target_properties(FBXSDK::FBXSDK PROPERTIES
    IMPORTED_IMPLIB_RELWITHDEBINFO "${FBX_LIB_RELWITHDEBINFO}"
    IMPORTED_LOCATION_RELWITHDEBINFO "${FBX_DLL_RELWITHDEBINFO}"
  )
endif()
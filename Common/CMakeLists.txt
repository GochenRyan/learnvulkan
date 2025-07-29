project(Common)

file(GLOB_RECURSE public_src CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE private_src CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(Command SHARED ${public_src} ${private_src})
setup_output_directories(Common)

target_include_directories(Common PUBLIC include)

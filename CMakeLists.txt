cmake_minimum_required(VERSION 3.14)
project(tmlcs_tui VERSION 0.5.0 LANGUAGES C)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Default to Release build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Find notcurses via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(NOTCURSES REQUIRED notcurses>=3.0)

# Collect source files
file(GLOB_RECURSE SRC_FILES src/*.c)

# Exclude main.c from library (contains main() — causes duplicate symbol errors)
list(FILTER SRC_FILES EXCLUDE REGEX "src/main\\.c")

# Exclude test/example files from library sources
list(FILTER SRC_FILES EXCLUDE REGEX "tests/")
list(FILTER SRC_FILES EXCLUDE REGEX "examples/")

# Static library
add_library(tmlcs_tui STATIC ${SRC_FILES})
target_include_directories(tmlcs_tui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/tmlcs-tui>
)
target_link_libraries(tmlcs_tui PUBLIC ${NOTCURSES_LIBRARIES} pthread m)

# Shared library with versioning
add_library(tmlcs_tui_shared SHARED ${SRC_FILES})
set_target_properties(tmlcs_tui_shared PROPERTIES
    OUTPUT_NAME tmlcs_tui
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)
target_include_directories(tmlcs_tui_shared PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/tmlcs-tui>
)
target_link_libraries(tmlcs_tui_shared PUBLIC ${NOTCURSES_LIBRARIES} pthread m)

# Optional tests
option(BUILD_TESTS "Build test suite" ON)
if(BUILD_TESTS)
    enable_testing()
    file(GLOB TEST_SRCS tests/test_*.c)
    foreach(TEST_SRC ${TEST_SRCS})
        get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
        add_executable(${TEST_NAME}_runner ${TEST_SRC})
        target_include_directories(${TEST_NAME}_runner PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_link_libraries(${TEST_NAME}_runner PRIVATE tmlcs_tui)
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME}_runner)
    endforeach()
endif()

# Install targets
include(GNUInstallDirs)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tmlcs-tui)
install(TARGETS tmlcs_tui tmlcs_tui_shared
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES tmlcs-tui.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# Copyright (c) 2022 Sebastian Pipping <sebastian@pipping.org>
# Licensed under the GPL v2 or later

cmake_minimum_required(VERSION 3.16.3)

project(omemo
    VERSION
        0.8.0
    # NOTE: Because this^^ version affects shared library filenames,
    #       it needs a major version bump to 1.0.0 already at
    #       the _first ever ABI break_ despite semver rule 4
    #       (https://semver.org/#spec-item-4).
    LANGUAGES
        C
)

include(FindPkgConfig)
include(GNUInstallDirs)


#
# Public configuration
#
option(BUILD_SHARED_LIBS "Build shared libraries (rather than static ones)" ON)
option(OMEMO_INSTALL "Install build artifacts" ON)
option(OMEMO_WITH_TESTS "Build test suite (depends on cmocka)" ON)
if(NOT _OMEMO_HELP)  # hide from "cmake -DOMEMO_HELP=ON -LH ." output
    option(_OMEMO_WARNINGS_AS_ERRORS "(Unofficial!) Turn warnings into errors" OFF)
    option(_OMEMO_WITH_COVERAGE "(Unofficial!) Build with coverage" OFF)
endif()

if(NOT BUILD_SHARED_LIBS)
    # NOTE: If we don't enforce -fPIC for static(!) libraries, we may run into
    #       "[..] relocation R_X86_64_PC32 against symbol [..]" link errors
    #       in dependent projects trying to link a shared library based on
    #       our static library.
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()


#
# Global CPPFLAGS and CFLAGS
#
add_compile_definitions(
    _XOPEN_SOURCE=700
    _DEFAULT_SOURCE
)
add_compile_options(
    -std=c99
    -Wall
    -Wextra
    -Wpedantic
    -Wstrict-overflow
    -fno-strict-aliasing
    -funsigned-char
    -fno-builtin-memset
)

if(_OMEMO_WARNINGS_AS_ERRORS)
    add_compile_options(-Werror)
endif()

if(_OMEMO_WITH_COVERAGE)
    set(_OMEMO_COVERAGE_FLAGS -g -O0 --coverage)
    add_compile_options(${_OMEMO_COVERAGE_FLAGS})
    link_libraries(${_OMEMO_COVERAGE_FLAGS})
endif()


#
# Build dependencies
#
# NOTE: We cannot use "pkg_check_modules([..] IMPORTED_TARGET [..])"
#       because we'd run into a (false positive) CMake error
#       "contains relative path in its INTERFACE_INCLUDE_DIRECTORIES"
#       when using "target_link_libraries([..] PkgConfig::[..])" with msys2.
if(OMEMO_WITH_TESTS)
    pkg_check_modules(CMOCKA REQUIRED "cmocka")
endif()
pkg_check_modules(GLIB REQUIRED "glib-2.0")
pkg_check_modules(GCRYPT REQUIRED "libgcrypt")
pkg_check_modules(MXML REQUIRED "mxml")
pkg_check_modules(SQLITE REQUIRED "sqlite3")


#
# C library
#
file(GLOB _OMEMO_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/libomemo*.[ch])
add_library(omemo ${_OMEMO_SOURCES})
target_include_directories(omemo PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)

if(OMEMO_INSTALL)
    file(GLOB _OMEMO_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/src/libomemo*.h)
    target_include_directories(omemo PUBLIC $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libomemo>)
    install(FILES ${_OMEMO_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libomemo)
    install(TARGETS omemo EXPORT omemo
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

if(NOT WIN32)
    set_property(TARGET omemo PROPERTY VERSION ${PROJECT_VERSION})
    set_property(TARGET omemo PROPERTY SOVERSION ${PROJECT_VERSION_MAJOR})
    set_property(TARGET omemo PROPERTY NO_SONAME ${NO_SONAME})
endif()


#
# pkg-config/pkgconf file
#
set(_OMEMO_PKGCONF_EXEC_PREFIX ${CMAKE_INSTALL_PREFIX})
set(_OMEMO_PKGCONF_LIBDIR ${CMAKE_INSTALL_FULL_LIBDIR})
set(_OMEMO_PKGCONF_INCLUDEDIR ${CMAKE_INSTALL_FULL_INCLUDEDIR})
set(_OMEMO_PKGCONF_PREFIX ${CMAKE_INSTALL_PREFIX})
string(REPLACE ${CMAKE_INSTALL_PREFIX} \${exec_prefix} _OMEMO_PKGCONF_LIBDIR ${_OMEMO_PKGCONF_LIBDIR})
string(REPLACE ${CMAKE_INSTALL_PREFIX} \${prefix} _OMEMO_PKGCONF_EXEC_PREFIX ${_OMEMO_PKGCONF_EXEC_PREFIX})
string(REPLACE ${CMAKE_INSTALL_PREFIX} \${prefix} _OMEMO_PKGCONF_INCLUDEDIR ${_OMEMO_PKGCONF_INCLUDEDIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/libomemo.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/libomemo.pc @ONLY)
set_target_properties(omemo PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/libomemo.pc)

if(OMEMO_INSTALL)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libomemo.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )
endif()


#
# C test suite
#
if(OMEMO_WITH_TESTS)
    set(_OMEMO_TEST_TARGETS test_crypto test_libomemo test_storage)

    enable_testing()

    foreach(_target ${_OMEMO_TEST_TARGETS})
        add_executable(${_target} ${CMAKE_CURRENT_SOURCE_DIR}/test/${_target}.c)
        target_link_libraries(${_target} PRIVATE omemo)
        add_test(NAME ${_target} COMMAND ${_target})

        if(BUILD_SHARED_LIBS)
            target_compile_options(${_target} PRIVATE ${CMOCKA_CFLAGS})
            target_link_libraries(${_target} PRIVATE ${CMOCKA_LIBRARIES})
        else()
            target_compile_options(${_target} PRIVATE ${CMOCKA_STATIC_CFLAGS})
            target_link_libraries(${_target} PRIVATE ${CMOCKA_STATIC_LIBRARIES})
        endif()
    endforeach()
endif()


#
# External build dependencies
#
foreach(_target omemo ${_OMEMO_TEST_TARGETS})
    if(BUILD_SHARED_LIBS)
        # TODO: Tests should stop depending on gcrypt
        #       once the tests stop including libomemo's .c(!) files
        target_compile_options(${_target} PRIVATE ${GCRYPT_CFLAGS})
        target_link_libraries(${_target} PRIVATE ${GCRYPT_LIBRARIES})

        target_compile_options(${_target} PUBLIC ${GLIB_CFLAGS})
        target_link_libraries(${_target} PUBLIC ${GLIB_LIBRARIES})

        # TODO: Tests should stop depending on mxml
        #       once the tests stop including libomemo's .c(!) files
        target_compile_options(${_target} PRIVATE ${MXML_CFLAGS})
        target_link_libraries(${_target} PRIVATE ${MXML_LIBRARIES})

        target_compile_options(${_target} PRIVATE ${SQLITE_CFLAGS})
        target_link_libraries(${_target} PRIVATE ${SQLITE_LIBRARIES})
    else()
        # TODO: Tests should stop depending on gcrypt
        #       once the tests stop including libomemo's .c(!) files
        target_compile_options(${_target} PRIVATE ${GCRYPT_STATIC_CFLAGS})
        target_link_libraries(${_target} PRIVATE ${GCRYPT_STATIC_LIBRARIES})

        target_compile_options(${_target} PUBLIC ${GLIB_STATIC_CFLAGS})
        target_link_libraries(${_target} PUBLIC ${GLIB_STATIC_LIBRARIES})

        # TODO: Tests should stop depending on mxml
        #       once the tests stop including libomemo's .c(!) files
        target_compile_options(${_target} PRIVATE ${MXML_STATIC_CFLAGS})
        target_link_libraries(${_target} PRIVATE ${MXML_STATIC_LIBRARIES})

        target_compile_options(${_target} PRIVATE ${SQLITE_STATIC_CFLAGS})
        target_link_libraries(${_target} PRIVATE ${SQLITE_STATIC_LIBRARIES})
    endif()
endforeach()


#
# Coverage reporting
#
if(_OMEMO_WITH_COVERAGE)
    add_custom_target(coverage
        COMMAND gcovr -r ${CMAKE_CURRENT_SOURCE_DIR} --html --html-details -o coverage.html
        COMMAND gcovr -r ${CMAKE_CURRENT_SOURCE_DIR} -s
    )
endif()

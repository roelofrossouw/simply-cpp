include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)
include(CMakeParseArguments)

set(SC_VERSION_FILE "VERSION.txt")
set(SC_VERSION_DEFAULT "1.0.0")

function(parse_sc_version text output)
    string(REGEX REPLACE "^[vV]" "" candidate "${text}")
    # CMake regexes have no {m,n} repetition, so the optional parts are spelled out.
    if (candidate MATCHES "^[0-9]+(\\.[0-9]+)?(\\.[0-9]+)?(\\.[0-9]+)?$")
        set(${output} "${candidate}" PARENT_SCOPE)
    else ()
        set(${output} "" PARENT_SCOPE)
    endif ()
endfunction()

function(get_sc_version)
    set(version_file "${CMAKE_CURRENT_SOURCE_DIR}/${SC_VERSION_FILE}")
    set(version "")

    find_package(Git QUIET)
    if (Git_FOUND)
        execute_process(
                COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                OUTPUT_VARIABLE git_tag
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
                RESULT_VARIABLE git_result
        )

        if (git_result EQUAL 0)
            parse_sc_version("${git_tag}" version)
            if ("${version}" STREQUAL "")
                message(WARNING "Ignoring git tag '${git_tag}': not a version number")
            endif ()
        endif ()
    endif ()

    if (NOT "${version}" STREQUAL "")
        message(STATUS "Version ${version} from git tag '${git_tag}'")
        write_sc_version_file("${version}" "${version_file}")
    else ()
        read_sc_version_file("${version_file}" version)
    endif ()

    if ("${version}" STREQUAL "")
        set(version "${SC_VERSION_DEFAULT}")
        message(WARNING "No usable git tag and no ${SC_VERSION_FILE}, defaulting to version ${version}."
                " Configure once in a tagged checkout to create the file.")
    endif ()

    set(SC_VERSION "${version}" PARENT_SCOPE)
endfunction()

function(write_sc_version_file version version_file)
    set(staged "${CMAKE_CURRENT_BINARY_DIR}/${SC_VERSION_FILE}")
    file(WRITE "${staged}" "${version}\n")
    file(COPY_FILE "${staged}" "${version_file}" ONLY_IF_DIFFERENT RESULT copy_error)
    if (copy_error)
        message(WARNING "Could not update ${version_file}: ${copy_error}."
                " A build without git will fall back to whatever it already holds.")
    endif ()
endfunction()

function(read_sc_version_file version_file output)
    set(${output} "" PARENT_SCOPE)
    if (NOT EXISTS "${version_file}")
        return()
    endif ()

    file(READ "${version_file}" contents)
    string(STRIP "${contents}" contents)
    parse_sc_version("${contents}" version)
    if ("${version}" STREQUAL "")
        message(WARNING "Ignoring ${version_file}: '${contents}' is not a version number")
        return()
    endif ()

    message(STATUS "Version ${version} from ${SC_VERSION_FILE} (no usable git tag)")
    set(${output} "${version}" PARENT_SCOPE)
endfunction()

function(find_or_install_package package apt_name brew_name)
    message(STATUS "Detecting ${package}")
    find_package(${package} QUIET)
    if (NOT ${package}_FOUND)
        if (UNIX AND EXISTS "/usr/bin/apt")
            message(STATUS "${package} not found, attempting apt installation...")
            execute_process(COMMAND sudo apt -y install ${apt_name} RESULT_VARIABLE INSTALL_RESULT)
        endif ()
        if (APPLE)
            message(STATUS "${package} not found, attempting brew installation...")
            execute_process(COMMAND brew install ${brew_name} RESULT_VARIABLE INSTALL_RESULT)
        endif ()
        find_package(CURL QUIET)
        if (NOT CURL_FOUND)
            message(FATAL_ERROR "Failed to install or locate ${package} (install result=${INSTALL_RESULT})")
        endif ()
    else ()
        message(STATUS "${package} found - ${${package}_VERSION}")
    endif ()
endfunction()

function(add_sc_object object)
    set(object_name "sc-${object}")
    set(source_file "src/${object}.cpp")
    set(header_file "include/${object}.h")

    set(options)
    set(one_value_args)
    set(multi_value_args LINK_LIBRARIES PUBLIC_LINK_LIBRARIES)
    cmake_parse_arguments(SC_OBJECT "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
    set(INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    add_library(${object_name} OBJECT ${source_file} ${header_file})
    set_target_properties(${object_name} PROPERTIES EXCLUDE_FROM_ALL ON)
    target_include_directories(${object_name} PRIVATE ${INCLUDE})

    if (SC_OBJECT_LINK_LIBRARIES)
        target_link_libraries(${object_name} PRIVATE ${SC_OBJECT_LINK_LIBRARIES})
    endif ()

    list(APPEND SOURCE_OBJECTS $<TARGET_OBJECTS:${object_name}>)
    set(SOURCE_OBJECTS "${SOURCE_OBJECTS}" PARENT_SCOPE)

    if (SC_OBJECT_PUBLIC_LINK_LIBRARIES)
        list(APPEND SOURCE_LINK_LIBRARIES ${SC_OBJECT_PUBLIC_LINK_LIBRARIES})
        set(SOURCE_LINK_LIBRARIES "${SOURCE_LINK_LIBRARIES}" PARENT_SCOPE)
    endif ()
endfunction()

# add_sc_test(<name> [TIMEOUT <seconds>] [LABELS <label>...])
#
# Builds tests/<name>.cpp into test-<name> and registers it with ctest.
# Tests report every failed check on stderr and exit non-zero (see tests/sc_test.h).
function(add_sc_test name)
    set(options)
    set(one_value_args TIMEOUT)
    set(multi_value_args LABELS)
    cmake_parse_arguments(SC_TEST "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if (NOT SC_TEST_TIMEOUT)
        set(SC_TEST_TIMEOUT 120)
    endif ()

    set(target "test-${name}")
    add_executable(${target} "${name}.cpp" sc_test.h)
    target_link_libraries(${target} PRIVATE sc)
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    add_test(NAME ${target} COMMAND ${target})
    set_tests_properties(${target} PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
            TIMEOUT ${SC_TEST_TIMEOUT}
            LABELS "${SC_TEST_LABELS}")
endfunction()
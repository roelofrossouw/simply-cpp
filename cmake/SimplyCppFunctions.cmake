include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(FetchContent)
include(CMakeParseArguments)

function(get_sc_version)
    find_package(Git QUIET)
    if (Git_FOUND)
        execute_process(
                COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                OUTPUT_VARIABLE SC_VERSION
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
                RESULT_VARIABLE GIT_RESULT
        )

        if (GIT_RESULT EQUAL 0)
            string(REGEX REPLACE "^v" "" SC_VERSION "${SC_VERSION}")
        else ()
            set(SC_VERSION "1.0.0")
        endif ()
    else ()
        set(SC_VERSION "1.0.0")
    endif ()
    set(SC_VERSION "${SC_VERSION}" PARENT_SCOPE)
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
    else()
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

function(add_sc_test name)
    add_executable("test-${name}" "${name}.cpp")
    target_link_libraries("test-${name}" PRIVATE sc)
    add_test(NAME "test-${name}" COMMAND "test-${name}")
endfunction()
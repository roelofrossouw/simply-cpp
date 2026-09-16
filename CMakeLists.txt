cmake_minimum_required(VERSION 3.22)
set(SC_MODULE core)
project(simply-cpp-${CS_MODULE} LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Options:
option(SC_CORE_BUILD_TESTS "Build simply-cpp-core tests" ${PROJECT_IS_TOP_LEVEL})

include(cmake/SimplyCppFunctions.cmake)
get_sc_version()
message(STATUS "SimplyCPP ${SC_MODULE} Version: ${SC_VERSION}")

# Core owns scripts/*.in, so it configures its own copies directly. Every other module
# gets the same two scripts through sc_bootstrap.cmake.
set(SC_DEPLOY_NAME "${SC_MODULE}" CACHE STRING "Directory this module is deployed to on the server")
set(SC_SCRIPT_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(GLOB SC_SCRIPT_TEMPLATES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.sh.in)
foreach (template ${SC_SCRIPT_TEMPLATES})
    get_filename_component(script ${template} NAME_WLE) # deploy.sh.in -> deploy.sh
    configure_file(${template} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/${script} @ONLY FILE_PERMISSIONS ${SC_SCRIPT_PERMISSIONS})
endforeach ()

find_or_install_package(CURL libcurl4-openssl-dev curl)

message(STATUS "Fetching nlohmann/json")
FetchContent_Declare(json GIT_REPOSITORY https://github.com/nlohmann/json.git GIT_TAG v3.12.0 GIT_SHALLOW TRUE EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(json)


###########
# Objects #
###########
set(SOURCE_OBJECTS include/sc.h)
set(SOURCE_LIBRARIES)
set(SOURCE_LINK_LIBRARIES)

add_sc_object(core)
add_sc_object(utf8)
add_sc_object(base64)
add_sc_object(timer)
add_sc_object(color)
add_sc_object(rect)
add_sc_object(date)
add_sc_object(percent)
add_sc_object(ollama LINK_LIBRARIES nlohmann_json::nlohmann_json)
add_sc_object(rest LINK_LIBRARIES curl PUBLIC_LINK_LIBRARIES curl)

######################
# Exported Libraries #
######################
add_sc_libraries(sc DESCRIPTION "simply-cpp-core")

################
# Installation #
################
install_sc_module(sc PATH_VARS CMAKE_INSTALL_INCLUDEDIR)

# Core only: what every other module picks up through find_package(sc) or the bootstrap
# instead of keeping its own copy.
install(FILES cmake/SimplyCppFunctions.cmake cmake/sc_bootstrap.cmake
        ${SC_SCRIPT_TEMPLATES}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/sc)
install(FILES tests/sc_test.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

#########
# Tests #
#########
if (SC_CORE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests sc-test)
endif ()
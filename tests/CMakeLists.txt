# Resources for Tests #
set(RESOURCE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resource")
set(RESOURCE_DESTINATION_DIR "${CMAKE_CURRENT_BINARY_DIR}/resource")
add_custom_target(copy_cs_resources ALL COMMAND ${CMAKE_COMMAND} -E copy_directory ${RESOURCE_SOURCE_DIR} ${RESOURCE_DESTINATION_DIR})

## TESTS ##
# What every test in this directory links against.
set(SC_TEST_LINK_LIBRARIES sc-core)

# Tests labelled "network" reach outside the build; run only the rest with ctest -LE network
add_sc_test(core LABELS unit)
add_sc_test(console LABELS unit)
add_sc_test(utf8 LABELS unit)
add_sc_test(date LABELS unit)
add_sc_test(datetime LABELS unit)
add_sc_test(base64 LABELS unit)
add_sc_test(timer LABELS unit)
add_sc_test(color LABELS unit)
add_sc_test(config LABELS unit)
add_sc_test(params LABELS unit)
# Built with the licence recorded, as add_sc_example() builds demos.
add_sc_test(params_license LABELS unit)
sc_program_info(test-params_license)
add_sc_test(log_level LABELS unit)

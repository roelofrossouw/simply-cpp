# Resources for Tests #
set(RESOURCE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resource")
set(RESOURCE_DESTINATION_DIR "${CMAKE_CURRENT_BINARY_DIR}/resource")
add_custom_target(copy_cs_resources ALL COMMAND ${CMAKE_COMMAND} -E copy_directory ${RESOURCE_SOURCE_DIR} ${RESOURCE_DESTINATION_DIR})

## TESTS ##
# Tests labelled "network" reach outside the build; run only the rest with ctest -LE network
add_sc_test(date LABELS unit)
add_sc_test(base64 LABELS unit)
add_sc_test(timer LABELS unit)
add_sc_test(color LABELS unit)
add_sc_test(rect LABELS unit)
add_sc_test(point LABELS unit)
add_sc_test(dual LABELS unit)
add_sc_test(percent LABELS unit)
add_sc_test(rest LABELS network TIMEOUT 120)
add_sc_test(ollama LABELS network TIMEOUT 600)
add_dependencies(test-ollama copy_cs_resources)

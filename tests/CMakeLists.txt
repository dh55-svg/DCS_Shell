include(${CMAKE_SOURCE_DIR}/cmake/GoogleTest.cmake)

function(add_unit_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE
        GTest::gtest_main
        GTest::gmock_main
        Qt6::Core
        Qt6::Test
        plugin_interface
    )
    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}/host
        ${CMAKE_SOURCE_DIR}
    )
    gtest_discover_tests(${name})
endfunction()

function(add_integration_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE
        GTest::gtest_main
        GTest::gmock_main
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
        Qt6::Test
        plugin_interface
    )
    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}/host
        ${CMAKE_SOURCE_DIR}
    )
    gtest_discover_tests(${name})
endfunction()

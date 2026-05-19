# Unit tests use Qt Test (built into Qt6, no external deps)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

function(add_unit_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE
        host_core
        Qt6::Core
        Qt6::Test
        plugin_interface
    )
    target_include_directories(${name} PRIVATE
        ${CMAKE_SOURCE_DIR}/host
        ${CMAKE_SOURCE_DIR}
    )
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "PATH=C:/Qt/6.5.3/mingw_64/bin;C:/Qt/Tools/mingw1120_64/bin;$ENV{PATH}"
    )
endfunction()

function(add_integration_test name)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE
        host_core
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
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "PATH=C:/Qt/6.5.3/mingw_64/bin;C:/Qt/Tools/mingw1120_64/bin;$ENV{PATH}"
    )
endfunction()

# ── Unit Tests ──
add_unit_test(test_alarm_state_machine unit/test_alarm_state_machine.cpp)
add_unit_test(test_deadband_filter unit/test_deadband_filter.cpp)
add_unit_test(test_deviation_checker unit/test_deviation_checker.cpp)
add_unit_test(test_rate_of_change_checker unit/test_rate_of_change_checker.cpp)
add_unit_test(test_double_buffer unit/test_double_buffer.cpp)
add_unit_test(test_lockfree_ring_buffer unit/test_lockfree_ring_buffer.cpp)
add_unit_test(test_tag_manager unit/test_tag_manager.cpp)
add_unit_test(test_json_config_repo unit/test_json_config_repo.cpp)
add_unit_test(test_flood_detector unit/test_flood_detector.cpp)
add_unit_test(test_chattering_guard unit/test_chattering_guard.cpp)
add_unit_test(test_shelve_manager unit/test_shelve_manager.cpp)
add_unit_test(test_app_config unit/test_app_config.cpp)
add_unit_test(test_plugin_hub unit/test_plugin_hub.cpp)
add_unit_test(test_suppression_engine unit/test_suppression_engine.cpp)
add_unit_test(test_alarm_engine unit/test_alarm_engine.cpp)
add_unit_test(test_performance performance/test_performance.cpp)

# ── Integration Tests ──
# AUTOMOC requires Q_OBJECT headers listed in target sources.
# Each test includes the mock header it uses + mocks_register.cpp for MOC generation.
add_integration_test(test_simulator_plugin_load
    integration/test_simulator_plugin_load.cpp
    mocks/mocks_register.cpp
    mocks/mock_fieldbus.h
)
add_integration_test(test_hot_swap
    integration/test_hot_swap.cpp
    mocks/mocks_register.cpp
)
add_integration_test(test_sqlite_persistence
    integration/test_sqlite_persistence.cpp
    mocks/mock_alarm_repo.h
    mocks/mock_history_repo.h
)
add_integration_test(test_mysql_persistence
    integration/test_mysql_persistence.cpp
)
add_integration_test(test_pipeline_with_simulator
    integration/test_pipeline_with_simulator.cpp
    mocks/mocks_register.cpp
    mocks/mock_fieldbus.h
)

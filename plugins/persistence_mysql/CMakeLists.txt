set(PLUGIN_NAME "MysqlPersistencePlugin")

add_library(${PLUGIN_NAME} SHARED
    MysqlPersistencePlugin.cpp
)

target_include_directories(${PLUGIN_NAME} PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/host)
target_link_libraries(${PLUGIN_NAME} PRIVATE
    plugin_interface
    Qt6::Core
    Qt6::Sql
)

set_target_properties(${PLUGIN_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    PREFIX ""
)

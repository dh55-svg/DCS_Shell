set(PLUGIN_NAME "QtMqttPlugin")

add_library(${PLUGIN_NAME} SHARED
    QtMqttPlugin.cpp
)

target_include_directories(${PLUGIN_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(${PLUGIN_NAME} PRIVATE
    plugin_interface
    Qt6::Core
    Qt6::Mqtt
)

set_target_properties(${PLUGIN_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    PREFIX ""
)

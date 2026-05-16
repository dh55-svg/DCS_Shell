set(PLUGIN_NAME "SimulatorPlugin")

add_library(${PLUGIN_NAME} SHARED
    SimulatorPlugin.cpp
    SimulatorImpl.cpp
)

target_include_directories(${PLUGIN_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(${PLUGIN_NAME} PRIVATE
    plugin_interface
    Qt6::Core
)

set_target_properties(${PLUGIN_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
    PREFIX ""
)

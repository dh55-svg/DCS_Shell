add_library(plugin_interface INTERFACE)
target_include_directories(plugin_interface INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(plugin_interface INTERFACE Qt6::Core)

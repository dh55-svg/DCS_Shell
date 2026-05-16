add_library(plugin_interface STATIC
    plugin_interface.cpp
    IFieldBus.h
    IMqttGateway.h
    IAlarmRepo.h
    IHistoryRepo.h
    ITagRepo.h
    IOperationRepo.h
    IMessageBus.h
    PluginSdk.h
)
target_include_directories(plugin_interface PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(plugin_interface PUBLIC Qt6::Core)

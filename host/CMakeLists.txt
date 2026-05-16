set(HOST_SOURCES
    main.cpp
    infrastructure/plugin/PluginHub.cpp
    infrastructure/config/JsonConfigRepo.cpp
    domain/tag/tagmanager.cpp
    domain/alarm/AlarmEngine.cpp
    domain/alarm/alarmchangelog.cpp
    domain/alarm/alarmkpimonitor.cpp
    pipeline/DataParseThread.cpp
    pipeline/DataPipeline.cpp
    pipeline/HistorySampler.cpp
    application/DataController.cpp
    application/AlarmController.cpp
    presentation/MainWindow.cpp
    presentation/models/AlarmTableModel.cpp
    presentation/models/TagDataModel.cpp
    presentation/models/DeviceStatusModel.cpp
    presentation/delegates/AlarmDelegate.cpp
    presentation/delegates/StatusDelegate.cpp
    presentation/delegates/ValueDelegate.cpp
)

add_executable(DCS_Shell ${HOST_SOURCES})

target_include_directories(DCS_Shell PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(DCS_Shell PRIVATE
    plugin_interface
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::Multimedia
)

install(TARGETS DCS_Shell RUNTIME DESTINATION .)

set(HOST_HEADERS
    infrastructure/nulls/NullFieldBus.h
    infrastructure/nulls/NullAlarmRepo.h
    infrastructure/nulls/NullHistoryRepo.h
    infrastructure/nulls/NullTagRepo.h
    infrastructure/nulls/NullOperationRepo.h
)

set(HOST_CORE_SOURCES
    ${HOST_HEADERS}
    infrastructure/plugin/PluginHub.cpp
    infrastructure/config/JsonConfigRepo.cpp
    domain/tag/tagmanager.cpp
    domain/alarm/AlarmEngine.cpp
    domain/alarm/alarmchangelog.cpp
    domain/alarm/alarmkpimonitor.cpp
    pipeline/DataParseThread.cpp
    pipeline/DataPipeline.cpp
    pipeline/HistorySampler.cpp
)

# Static library for domain + infrastructure (testable without Qt Widgets)
add_library(host_core STATIC ${HOST_CORE_SOURCES})
target_include_directories(host_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(host_core PUBLIC
    plugin_interface
    Qt6::Core
    Qt6::Sql
    Qt6::Network
)

set(HOST_APP_SOURCES
    main.cpp
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

add_executable(DCS_Shell ${HOST_APP_SOURCES})
target_include_directories(DCS_Shell PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
)
target_link_libraries(DCS_Shell PRIVATE
    host_core
    plugin_interface
    Qt6::Core
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
    Qt6::Multimedia
)

install(TARGETS DCS_Shell RUNTIME DESTINATION .)

set(HOST_SOURCES
    main.cpp
    infrastructure/plugin/PluginHub.cpp
    infrastructure/config/JsonConfigRepo.cpp
    domain/tag/tagmanager.cpp
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
)

install(TARGETS DCS_Shell RUNTIME DESTINATION .)

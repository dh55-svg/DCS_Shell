add_subdirectory(fieldbus_simulator)
add_subdirectory(persistence_sqlite)

# Modbus plugin: requires libmodbus
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBMODBUS libmodbus)
endif()
if(LIBMODBUS_FOUND)
    message(STATUS "libmodbus found — building ModbusPlugin")
    add_subdirectory(fieldbus_modbus)
else()
    message(STATUS "libmodbus not found — skipping ModbusPlugin")
endif()

# MQTT plugin: requires Qt6::Mqtt
if(Qt6Mqtt_FOUND)
    message(STATUS "Qt6::Mqtt found — building QtMqttPlugin")
    add_subdirectory(mqtt_qt)
else()
    message(STATUS "Qt6::Mqtt not found — skipping QtMqttPlugin")
endif()



file(GLOB SOURCES 
        "${CMAKE_CURRENT_LIST_DIR}/../src/*.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/../src/pico/*.cpp"
)

add_library(IoAbstraction INTERFACE ${SOURCES})

target_compile_definitions(IoAbstraction
        INTERFACE BUILD_FOR_PICO_CMAKE=1 BUILD_PICO_FORCE_UART=1 IO_LOGGING_DEBUG=1
)

target_include_directories(IoAbstraction INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/../src
)

target_link_libraries(IoAbstraction INTERFACE
        pico_stdlib pico_sync hardware_i2c hardware_spi hardware_adc hardware_pwm
        SimpleCollections TaskManagerIO)

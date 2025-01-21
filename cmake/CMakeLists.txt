cmake_minimum_required(VERSION 3.13)

add_library(IoAbstraction
        ../src/EepromAbstraction.cpp
        ../src/EepromAbstractionWire.cpp
        ../src/IoAbstraction.cpp
        ../src/IoAbstractionWire.cpp
        ../src/KeyboardManager.cpp
        ../src/ResistiveTouchScreen.cpp
        ../src/SwitchInput.cpp
        ../src/wireHelpers.cpp
        ../src/pico/PicoDigitalIO.cpp
        ../src/pico/i2cWrapper.cpp
        ../src/pico/picoAnalogDevice.cpp
)

target_compile_definitions(IoAbstraction
        PUBLIC BUILD_FOR_PICO_CMAKE=1 BUILD_PICO_FORCE_UART=1 IO_LOGGING_DEBUG=1
)

target_include_directories(IoAbstraction PUBLIC
        ../src
)

target_link_libraries(IoAbstraction PUBLIC
        pico_stdlib pico_sync hardware_i2c hardware_spi hardware_adc hardware_pwm
        SimpleCollections TaskManagerIO)

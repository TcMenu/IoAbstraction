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
        ../src/stmCube/CubeI2cWrapper.cpp
        ../src/stmCube/StmCubeDigital.cpp
)

if (BUILD_FOR_STM32CUBE)
    target_compile_definitions(IoAbstraction
            PUBLIC BUILD_FOR_STM32CUBE_CMAKE=1 IO_LOGGING_DEBUG=1
    )
else ()
    target_compile_definitions(IoAbstraction
            PUBLIC BUILD_FOR_PICO_CMAKE=1 BUILD_PICO_FORCE_UART=1 IO_LOGGING_DEBUG=1
    )
endif ()

target_include_directories(IoAbstraction PUBLIC
        ../src
)

target_link_libraries(IoAbstraction PUBLIC
        SimpleCollections TaskManagerIO)


# only bring in pico libs when compiling for PicoSDK
if(TARGET pico_stdlib AND TARGET pico_sync)
    message("IoAbstraction Build is pico, adding libs")
    target_link_libraries(IoAbstraction PUBLIC pico_stdlib pico_sync hardware_i2c hardware_spi hardware_adc hardware_pwm)
elseif (TARGET stm32cubemx)
    message("IoAbstraction Build is stm32cube adding stm32cubemx")
    target_link_libraries(IoAbstraction PUBLIC stm32cubemx)
endif()
cmake_minimum_required(VERSION 3.13)

add_library(IoAbstraction
        ../src/EepromAbstraction.cpp
        ../src/EepromAbstractionWire.cpp
        ../src/IoAbstraction.cpp
        ../src/IoAbstractionWire.cpp
        ../src/KeyboardManager.cpp
        ../src/ResistiveTouchScreen.cpp
        ../src/SwitchInput.cpp
        ../src/StateMachineEncoder.cpp
        ../src/wireHelpers.cpp
        ../src/pico/PicoDigitalIO.cpp
        ../src/pico/i2cWrapper.cpp
        ../src/pico/picoAnalogDevice.cpp
        ../src/stmCube/CubeI2cWrapper.cpp
        ../src/stmCube/StmCubeDigital.cpp
        ../src/mbed/HalStm32EepromAbstraction.cpp
        ../src/testing/TestDigitalIO.cpp
)

if (BUILD_FOR_STM32CUBE)
    target_compile_definitions(IoAbstraction
            PUBLIC BUILD_FOR_STM32CUBE_CMAKE=1 IO_LOGGING_DEBUG=1
    )
elseif (BUILD_NATIVE_TESTS)
    target_compile_definitions(IoAbstraction
            PUBLIC BUILD_FOR_NATIVE_PLATFORM=1 IO_LOGGING_DEBUG=1
    )
else ()
    target_compile_definitions(IoAbstraction
            PUBLIC BUILD_FOR_PICO_CMAKE=1 BUILD_PICO_FORCE_UART=1 IO_LOGGING_DEBUG=1
    )
endif ()

target_include_directories(IoAbstraction PUBLIC
        ../src
)

target_link_libraries(IoAbstraction PUBLIC TaskManagerIO)


# only bring in pico libs when compiling for PicoSDK
if(TARGET pico_stdlib AND TARGET pico_sync)
    message("IoAbstraction Build is pico, adding libs")
    target_link_libraries(IoAbstraction PUBLIC pico_stdlib pico_sync hardware_i2c hardware_spi hardware_adc hardware_pwm)
elseif (TARGET stm32cubemx)
    message("IoAbstraction Build is stm32cube adding stm32cubemx")
    target_link_libraries(IoAbstraction PUBLIC stm32cubemx)
endif()

if (BUILD_NATIVE_TESTS)
    get_unity_if_needed()
    add_executable(test_ioa_core
            ../test/test_ioa_core/ioaCoreTests.cpp
            ../test/test_ioa_core/ioDeviceTests.cpp
            ../test/test_ioa_core/negatingIoAbstractionTests.cpp
            ../test/test_ioa_core/switchesTests.cpp
            ../test/test_ioa_core/test_main.cpp
    )

    target_link_libraries(IoAbstraction PUBLIC)
    target_link_libraries(test_ioa_core PRIVATE unity TcMenuLog TaskManagerIO IoAbstraction)
    add_test(NAME test_ioa_core COMMAND test_ioa_core)
endif ()

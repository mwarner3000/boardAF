cmake_minimum_required(VERSION 3.20)

project(
    MicroSim
    VERSION 0.1.0
    DESCRIPTION "A modular embedded systems simulation framework"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(
    MicroSimCore
    src/Bus/Bus.cpp
    src/Memory/RAM.cpp
    src/Devices/GPIO.cpp
    src/Devices/Pin.cpp
    src/Devices/Timer.cpp
    src/Simulator/Simulator.cpp
    src/Simulator/Clock.cpp
    src/CPU/SimpleCPU.cpp
	src/Simulation/Simulation.cpp
	src/Interrupts/InterruptController.cpp
	src/Communication/CANBus.cpp
	src/Communication/CANController.cpp
	src/Simulation/SimulationScheduler.cpp
	src/Devices/ADC.cpp
)

target_include_directories(
    MicroSimCore
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

add_executable(
    MicroSim
    src/main.cpp
)

target_link_libraries(
    MicroSim
    PRIVATE
        MicroSimCore
)

add_executable(adc_gpio_interrupt_demo
    examples/adc_gpio_interrupt_demo.cpp
)

target_link_libraries(adc_gpio_interrupt_demo
    PRIVATE
    MicroSimCore
)

target_include_directories(
    MicroSim
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

enable_testing()

add_executable(
    RAMTests
    tests/RAMTests.cpp
)

target_link_libraries(
    RAMTests
    PRIVATE
        MicroSimCore
)

add_test(
    NAME RAMTests
    COMMAND RAMTests
)

add_executable(
    BusTests
    tests/BusTests.cpp
)

target_link_libraries(
    BusTests
    PRIVATE
        MicroSimCore
)

add_test(
    NAME BusTests
    COMMAND BusTests
)

add_executable(
    GPIOTests
    tests/GPIOTests.cpp
)

target_link_libraries(
    GPIOTests
    PRIVATE
        MicroSimCore
)

add_test(
    NAME GPIOTests
    COMMAND GPIOTests
)

add_executable(
    TimerTests
    tests/TimerTests.cpp
)

target_link_libraries(
    TimerTests
    PRIVATE
        MicroSimCore
)

add_test(
    NAME TimerTests
    COMMAND TimerTests
)

add_executable(
    CPUTests
    tests/CPUTests.cpp
)

target_link_libraries(
    CPUTests
    PRIVATE
        MicroSimCore
)

add_test(
    NAME CPUTests
    COMMAND CPUTests
)

add_executable(
    SimulationTests
    tests/SimulationTests.cpp
)

target_link_libraries(
    SimulationTests
    PRIVATE
        MicroSimCore
)

add_test(
    NAME SimulationTests
    COMMAND SimulationTests
)

add_executable(
    InterruptControllerTests
    tests/InterruptControllerTests.cpp
)

target_link_libraries(
    InterruptControllerTests
    PRIVATE
        MicroSimCore
)

add_test(
    NAME InterruptControllerTests
    COMMAND InterruptControllerTests
)

add_executable(
    CANFrameTests
    tests/CANFrameTests.cpp
)

target_link_libraries(
    CANFrameTests
    PRIVATE
        MicroSimCore
)

add_test(
    NAME CANFrameTests
    COMMAND CANFrameTests
)

add_executable(
    CANBusTests
    tests/CANBusTests.cpp
)

target_link_libraries(
    CANBusTests
    PRIVATE
        MicroSimCore
)

add_test(
    NAME CANBusTests
    COMMAND CANBusTests
)

add_executable(
    CANControllerTests
    tests/CANControllerTests.cpp
)

target_link_libraries(
    CANControllerTests
    PRIVATE
        MicroSimCore
)

add_test(
    NAME CANControllerTests
    COMMAND CANControllerTests
)

add_executable(
    ADCTests
    tests/ADCTests.cpp
)

target_link_libraries(
    ADCTests
    PRIVATE
        MicroSimCore
)

add_test(
    NAME ADCTests
    COMMAND ADCTests
)
cmake_minimum_required(VERSION 3.20)

project(
    boardAF
    VERSION 1.0.0
    DESCRIPTION "Board Abstraction Framework for embedded systems simulation"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(
    boardAFCore
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
    boardAFCore
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

add_executable(
    boardAF
    src/main.cpp
)

target_link_libraries(
    boardAF
    PRIVATE
        boardAFCore
)

add_executable(
    adc_gpio_interrupt_demo
    examples/adc_gpio_interrupt_demo.cpp
)

target_link_libraries(
    adc_gpio_interrupt_demo
    PRIVATE
        boardAFCore
)

target_include_directories(
    boardAF
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
        boardAFCore
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
        boardAFCore
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
        boardAFCore
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
        boardAFCore
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
        boardAFCore
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
        boardAFCore
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
        boardAFCore
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
        boardAFCore
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
        boardAFCore
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
        boardAFCore
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
        boardAFCore
)

add_test(
    NAME ADCTests
    COMMAND ADCTests
)
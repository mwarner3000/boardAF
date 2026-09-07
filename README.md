# boardAF

**Board Abstraction Framework**

boardAF is a modular microcontroller simulation framework written in C++.

The goal of boardAF is to provide configurable simulated microcontrollers that behave as self-contained embedded controllers. A simulated controller can execute firmware, interact with peripherals, receive external input signals, produce output signals, and respond to hardware interrupts without needing to know anything about the environment in which it is being used.

boardAF is intended for experimentation, prototyping, robotics simulation, embedded-system development, and testing generic microcontroller configurations.

## Project Goals

boardAF is designed around several core goals:

- Simulate configurable microcontroller hardware.
- Keep simulated hardware separate from assembly syntax, parsers, compilers, and other software toolchains.
- Keep the microcontroller independent from the external simulated world.
- Allow external environments to drive and observe controller pins through a small public interface.
- Support ordinary real-time execution without requiring the host application to calculate elapsed time itself.
- Preserve explicit cycle/time advancement for deterministic testing and simulation hosts that manage their own time.
- Support multiple independent simulated controllers in the same system.
- Allow future communication between controllers through interfaces such as CAN.
- Allow users to approximate different classes of microcontrollers by changing characteristics such as clock speed, RAM size, GPIO count, electrical parameters, and memory-map locations.
- Keep peripherals modular so additional devices can be added without redesigning the core simulator.
- Keep interrupt sources independent from CPU implementation details by routing IRQs through an interrupt controller.

boardAF is not intended to be an exact transistor-level, circuit-level, or brand-specific reproduction of a commercial microcontroller. Instead, it aims to model embedded-controller behavior and resource constraints at a useful level of abstraction.

## Current Status

boardAF is under active development. Implemented components currently include:

- System bus
- Configurable RAM size and base address
- GPIO with runtime-configurable pin count
- Voltage-based `Pin` model
- Pin-selected GPIO register interface
- Configurable GPIO and timer base addresses
- Timer peripheral
- Interrupt controller
- Memory-based interrupt vector table
- CPU interrupt entry and `RETI`
- Timer-generated interrupts
- Simulation clock
- Clock-frequency-based time advancement
- Real-time synchronization using a monotonic host clock
- Public pin-voltage accessors for external environments
- Simple CPU
- Simple instruction set
- Board configuration
- Multi-node simulation infrastructure
- Automated tests for major subsystems
- End-to-end external pin integration testing
- End-to-end Timer -> interrupt controller -> CPU -> handler -> `RETI` testing

The project should currently be considered experimental and its APIs may change during development.

## Hardware / Software Separation

boardAF separates simulated hardware from the software used to program it.

The CPU executes machine instructions defined by its instruction set. Assembly-language syntax is not part of the simulated hardware. This allows future assemblers, compilers, parsers, or other development tools to target a boardAF CPU without requiring changes to the CPU, peripherals, or other simulated hardware.

The current SimpleISA instruction set exists to develop and test the simulator and does not prevent other instruction sets or software toolchains from being added in the future.

## Time and Execution

One boardAF clock cycle represents one hardware clock cycle of the configured simulated controller.

`BoardConfig::clockHz` determines the relationship between elapsed time and MCU cycles. For example, advancing a 16 MHz controller by 1 ms executes 16,000 hardware cycles, while a 32 MHz controller executes 32,000 cycles during the same interval.

Advancing cycles does not skip hardware activity. The CPU and other clock-driven devices are advanced during those cycles.

boardAF currently supports three execution styles:

- `advanceCycles(n)` explicitly executes a known number of MCU cycles.
- `advanceTime(duration)` converts an elapsed duration into MCU cycles using the configured clock frequency.
- Real-time mode uses `std::chrono::steady_clock` to measure real elapsed host time and feeds that duration into the same time-advancement mechanism through `startRealTime()`, `updateRealTime()`, and `stopRealTime()`.

Real-time mode measures elapsed time rather than host processor cycles, so MCU timing is not based on how many CPU cycles the host machine happens to execute. The host application only needs to call `updateRealTime()` from its normal loop; it does not calculate elapsed time itself.

Explicit cycle and duration advancement remain useful for deterministic tests and for external simulators that already manage their own simulation time.

Firmware does not directly read the host clock. It experiences time through simulated MCU execution and peripherals such as timers.

## External Simulation Boundary

A boardAF controller does not model the physical world around it.

For example, boardAF does not need to know whether an input voltage represents wheel speed, temperature, pressure, a switch, a sensor, or some other world quantity. The external environment supplies pin voltages, firmware determines how those values are used, and the external environment decides what controller outputs affect.

The current world-facing pin interface is deliberately small:

```cpp
bot.setPinVoltage(pin, voltage);
double voltage = bot.getPinVoltage(pin);
```

For an ordinary real-time host loop, this can be combined with:

```cpp
bot.startRealTime();

while (running)
{
    bot.setPinVoltage(inputPin, inputVoltage);
    bot.updateRealTime();
    double outputVoltage = bot.getPinVoltage(outputPin);
}

bot.stopRealTime();
```

An external simulator with its own time-management system can instead call `advanceTime()`, while deterministic tests can call `advanceCycles()`.

The external environment does not need to know boardAF's GPIO register map, CPU registers, instruction encoding, or internal peripheral organization merely to exchange physical pin signals with the controller.

An end-to-end switched-light integration test verifies this boundary: the external side changes a switch pin voltage and observes a light pin voltage, while firmware inside the simulated controller performs the GPIO reads, decision making, and GPIO writes.

## GPIO and Pins

GPIO uses a runtime-sized `std::vector<Pin>`. The number of pins is therefore a board configuration property rather than being limited by a 32-bit register width.

The generic GPIO peripheral currently uses a pin-selected register interface:

| Offset | Register | Behavior |
| --- | --- | --- |
| 0 | `PIN_SELECT` | Selects the pin addressed by subsequent GPIO operations |
| 1 | `DIRECTION` | Reads or sets the selected pin direction |
| 2 | `OUTPUT` | Reads or drives the selected output pin LOW/HIGH |
| 3 | `INPUT` | Reads the selected pin as LOW/HIGH and is CPU read-only |

This allows a generic board to expose 8, 100, or more pins without dividing the user-visible GPIO model into artificial 32-pin banks. CPU word width remains an implementation property and does not define the total number of simulated pins.

Pins carry simulated voltage values. Digital HIGH/LOW interpretation uses the board's configured logic voltage and digital HIGH threshold. Detailed circuit behavior and the physical meaning of those voltages remain outside boardAF's core scope.

## Timer and Interrupts

The Timer is memory mapped and clock driven. Its current register interface is:

| Offset | Register | Behavior |
| --- | --- | --- |
| 0 | `COUNTER` | Current timer count |
| 1 | `PERIOD` | Expiration period |
| 2 | `ENABLE` | Enables/disables counting |
| 3 | `EXPIRED` | Expiration flag; writing 1 clears it |
| 4 | `INTERRUPT_ENABLE` | Enables/disables IRQ generation on expiration |

When the Timer expires with interrupts enabled, it requests its configured IRQ from `InterruptController`. This interrupt path is separate from memory-mapped bus traffic:

```text
Timer registers <-> Bus <-> CPU
       |
       +-- IRQ --> InterruptController --> CPU
```

`InterruptController` records pending interrupt numbers without knowing which peripheral they represent. The CPU checks for a pending IRQ between instructions, saves the current program counter, reads the handler address from its memory-based vector table, and begins executing the handler. SimpleISA's `RETI` instruction restores the saved return address and resumes interrupted firmware.

The current SimpleCPU interrupt implementation is intentionally basic. Interrupts do not nest, lower-numbered pending IRQs are selected first, only the return program counter is automatically preserved, and `HALT` currently prevents the CPU from checking for new interrupts. More advanced interrupt semantics can be added when required by a concrete board/CPU model.

## Configurable Boards

`BoardConfig` currently provides configuration for:

- CPU clock frequency
- RAM size
- GPIO pin count
- logic voltage
- digital HIGH threshold
- RAM base address
- GPIO base address
- timer base address

The long-term goal is to let users create generic board configurations approximating the resources and performance characteristics needed for a real embedded project without requiring boardAF to reproduce a particular commercial MCU exactly.

## Building

boardAF uses CMake and requires a C++20-compatible compiler.

From the repository root:

```sh
cmake -S . -B build
cmake --build build
```

On systems where a specific CMake generator is required, select the appropriate generator when configuring the build.

## Running Tests

After building:

```sh
ctest --test-dir build --output-on-failure
```

The current test targets cover RAM, Bus, GPIO, Timer, CPU, Simulation behavior, interrupt-controller behavior, real-time mode state, external pin access, the end-to-end external-world GPIO path, and the end-to-end Timer interrupt path.

## Project Structure

```text
boardAF/
├── docs/       Project design and architecture documentation
├── include/    Public headers
├── src/        Implementation source
├── tests/      Automated tests
├── CMakeLists.txt
└── README.md
```

## Future Direction

Likely future development includes:

- Refinement of the external integration API as additional hardware interfaces are added
- More configurable peripheral construction
- Additional timers and peripherals
- Analog input / ADC support
- PWM and other pin functions
- More advanced interrupt behavior where needed
- CAN or similar controller-to-controller communication
- Additional CPU architectures or instruction sets
- External assembler/compiler tooling
- More flexible board profiles and peripheral layouts
- DMA and multiple bus masters where required

These items describe project direction and should not be assumed to be implemented unless documented otherwise.

## Scope

boardAF focuses on the simulated embedded controller.

Detailed physical-world simulation, vehicle dynamics, robotics physics, circuit simulation, sensor physics, and similar environment-specific behavior are outside the core scope of the project. Those systems can instead interact with boardAF through its external interfaces.
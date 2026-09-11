# boardAF

**Board Abstraction Framework**

boardAF is a modular microcontroller simulation framework written in C++.

The goal of boardAF is to provide configurable simulated microcontrollers that behave as self-contained embedded controllers. A simulated controller can execute firmware, interact with peripherals, receive external input signals, produce output signals, and respond to hardware interrupts without needing to know anything about the environment in which it is being used.

boardAF is intended for experimentation, prototyping, robotics simulation, embedded-system development, and testing generic microcontroller configurations.

## Project Goals

boardAF is designed around several core goals:

* Simulate configurable microcontroller hardware.
* Keep simulated hardware separate from assembly syntax, parsers, compilers, and other software toolchains.
* Keep the microcontroller independent from the external simulated world.
* Allow external environments to drive and observe controller pins through a small public interface.
* Support ordinary real-time execution without requiring the host application to calculate elapsed time itself.
* Preserve explicit cycle/time advancement for deterministic testing and simulation hosts that manage their own time.
* Allow users to approximate different classes of microcontrollers by changing characteristics such as clock speed, RAM size, CPU register count, GPIO count, peripheral configuration, electrical parameters, interrupt count, and memory-map locations.
* Keep peripherals modular so additional devices can be added without redesigning the core simulator.
* Keep interrupt sources independent from CPU implementation details by routing IRQs through an interrupt controller.
* Preserve a clean boundary between simulated hardware and the software used to program it.

boardAF is not intended to be an exact transistor-level, circuit-level, or brand-specific reproduction of a commercial microcontroller. Instead, it aims to model embedded-controller behavior and resource constraints at a useful level of abstraction.

## Current Status — v1.0.0

boardAF v1.0.0 is the first usable proof-of-concept release of the framework.

The current implementation provides enough functionality to configure a basic simulated microcontroller, load machine-code firmware, execute that firmware, interact with external signals, use memory-mapped peripherals, and respond to hardware interrupts.

Implemented components include:

* System bus
* Configurable word-addressed RAM
* Configurable RAM base address
* GPIO with runtime-configurable pin count
* Voltage-based `Pin` model
* Floating external pin state
* Pin-selected GPIO register interface
* Configurable logic voltage and digital HIGH threshold
* Timer peripherals
* ADC peripherals with configurable channels, resolution, reference voltage, and conversion time
* Interrupt controller
* Configurable interrupt count
* Level-sensitive peripheral interrupt lines
* Memory-based interrupt vector table
* CPU interrupt entry and `RETI`
* Timer-generated interrupts
* ADC-generated interrupts
* CPU `HALT` and interrupt wake-up
* Simulation clock
* Configurable clock frequency
* Clock-frequency-based time advancement
* Real-time synchronization using a monotonic host clock
* Public pin-voltage accessors for external environments
* Simple CPU
* Simple machine instruction set
* Configurable CPU register count
* Board configuration
* Firmware loading
* Multiple Timer and ADC peripheral instances
* Automated tests for major subsystems
* End-to-end GPIO integration testing
* End-to-end Timer interrupt testing
* End-to-end ADC -> interrupt -> CPU -> GPIO firmware testing
* Standalone ADC/GPIO interrupt demonstration

v1.0.0 should be considered a functional proof of concept rather than a finished MCU ecosystem. APIs and internal architecture may evolve in future releases.

## Hardware / Software Separation

boardAF separates simulated hardware from the software used to program it.

The CPU executes machine instructions defined by its instruction set. Assembly-language syntax is not part of the simulated hardware. This allows future assemblers, compilers, parsers, or other development tools to target a boardAF CPU without requiring changes to the CPU, peripherals, or other simulated hardware.

The current SimpleISA instruction set exists to provide a usable CPU for developing and testing the simulator. It does not require firmware to be represented by any particular assembly-language syntax, and it does not prevent other CPU architectures or software toolchains from being added in the future.

Firmware can be supplied directly as encoded machine instructions through the simulator's firmware-loading interface.

## Firmware Loading

Firmware is represented as a sequence of 32-bit machine words and can be loaded with:

```cpp
std::vector<std::uint32_t> firmware = {
    // encoded machine instructions
};

Simulator simulator;
simulator.loadFirmware(firmware);
```

The current SimpleCPU resets its program counter to address `0`, so executable v1 board configurations require RAM to begin at address `0`.

`loadFirmware()` verifies that the firmware image fits within the configured RAM before loading it.

Assemblers, compilers, linkers, and executable file formats are intentionally outside the simulated hardware layer. Future development tools can generate the same machine-code representation without requiring changes to the simulator.

## SimpleCPU and SimpleISA

boardAF currently includes SimpleCPU and SimpleISA as its reference CPU implementation.

SimpleISA currently provides instructions for:

* Immediate values and register movement
* Direct memory loads and stores
* Addition and subtraction
* Equality comparison
* Conditional and unconditional jumps
* Interrupt return
* CPU halt

The current instruction encoding uses:

* 8-bit opcode
* 4-bit destination register field
* 4-bit source register field
* 16-bit operand field

The CPU register count is configurable from 1 to 16 registers.

SimpleCPU uses direct 16-bit addresses and operands, giving SimpleISA direct access to addresses in the `0x0000`–`0xFFFF` range.

SimpleISA is intentionally small. It exists to make boardAF usable and to prove the hardware/software boundary rather than to reproduce a particular commercial CPU architecture.

## Time and Execution

One boardAF clock cycle represents one hardware clock cycle of the configured simulated controller.

`BoardConfig::clockHz` determines the relationship between elapsed time and MCU cycles. v1 supports configured clock frequencies from **1 Hz through 1 GHz**.

For example, advancing a 16 MHz controller by 1 ms executes 16,000 hardware cycles, while a 32 MHz controller executes 32,000 cycles during the same interval.

Advancing cycles does not skip hardware activity. The CPU and other clock-driven devices are advanced during those cycles.

boardAF supports several execution styles:

* `run(n)` executes a maximum of `n` MCU cycles.
* `advanceCycles(n)` explicitly executes a known number of MCU cycles.
* `advanceTime(duration)` converts elapsed simulated time into MCU cycles using the configured clock frequency.
* Real-time mode uses `std::chrono::steady_clock` to measure real elapsed host time and feeds that duration into the same time-advancement mechanism through `startRealTime()`, `updateRealTime()`, and `stopRealTime()`.

Real-time mode measures elapsed time rather than host processor cycles, so MCU timing is not based on how many CPU cycles the host machine happens to execute. The host application only needs to call `updateRealTime()` from its normal loop; it does not calculate elapsed time itself.

Explicit cycle and duration advancement remain useful for deterministic tests and for external simulators that already manage their own simulation time.

Firmware does not directly read the host clock. It experiences time through simulated MCU execution and peripherals such as timers.

## External Simulation Boundary

A boardAF controller does not model the physical world around it.

For example, boardAF does not need to know whether an input voltage represents wheel speed, temperature, pressure, a switch, a sensor, or some other world quantity. The external environment supplies pin voltages, firmware determines how those values are used, and the external environment decides what controller outputs affect.

The world-facing pin interface is deliberately small:

```cpp
bot.setPinVoltage(pin, voltage);
auto voltage = bot.getPinVoltage(pin);
```

An external simulator can therefore interact with a boardAF controller using physical-style signals without needing to know its GPIO register map, CPU registers, instruction encoding, or internal peripheral organization.

For an ordinary real-time host loop, this can be combined with:

```cpp
bot.startRealTime();

while (running)
{
    bot.setPinVoltage(inputPin, inputVoltage);
    bot.updateRealTime();

    auto outputVoltage = bot.getPinVoltage(outputPin);
}

bot.stopRealTime();
```

An external simulator with its own time-management system can instead call `advanceTime()`, while deterministic tests can call `advanceCycles()`.

This separation allows the same simulated controller to operate inside different external environments without embedding world-specific behavior into boardAF itself.

## GPIO and Pins

GPIO uses a runtime-sized collection of `Pin` objects. The number of pins is therefore a board configuration property rather than being limited by CPU word width.

The generic GPIO peripheral uses a pin-selected register interface:

| Offset | Register     | Behavior                                                |
| ------ | ------------ | ------------------------------------------------------- |
| 0      | `PIN_SELECT` | Selects the pin addressed by subsequent GPIO operations |
| 1      | `DIRECTION`  | Reads or sets the selected pin direction                |
| 2      | `OUTPUT`     | Reads or drives the selected output pin LOW/HIGH        |
| 3      | `INPUT`      | Reads the selected pin as LOW/HIGH and is CPU read-only |

This allows a generic board to expose a configurable number of pins without dividing the user-visible GPIO model into artificial fixed-width banks.

Each pin maintains state intrinsic to the simulated MCU pin, including its direction, output latch, and optional externally supplied voltage.

Digital HIGH/LOW interpretation uses the board's configured logic voltage and digital HIGH threshold.

Detailed circuit behavior and the physical meaning of those voltages remain outside boardAF's core scope.

## ADC

boardAF v1 includes configurable analog-to-digital converters.

Each ADC instance represents one converter with multiple selectable input channels. Board configuration determines characteristics such as:

* Channel count
* Resolution
* Reference voltage
* Conversion time
* Interrupt number
* Memory-mapped base address
* Channel-to-pin mapping

The current ADC register interface is:

| Offset | Register           |
| ------ | ------------------ |
| 0      | `CHANNEL`          |
| 1      | `CONTROL`          |
| 2      | `STATUS`           |
| 3      | `RESULT`           |
| 4      | `RESULT_CHANNEL`   |
| 5      | `INTERRUPT_ENABLE` |

Writing the START bit in `CONTROL` begins a conversion.

The ADC samples its selected pin when the conversion begins and produces the digital result after its configured conversion time.

The ADC supports COMPLETE, OVERRUN, and INVALID_CHANNEL status conditions. Status conditions that require firmware acknowledgement use write-one-to-clear behavior.

When COMPLETE is active and ADC interrupts are enabled, the ADC asserts its configured interrupt line.

## Timer and Interrupts

The Timer is memory mapped and clock driven. Its current register interface is:

| Offset | Register           | Behavior                                      |
| ------ | ------------------ | --------------------------------------------- |
| 0      | `COUNTER`          | Current timer count                           |
| 1      | `PERIOD`           | Expiration period                             |
| 2      | `ENABLE`           | Enables/disables counting                     |
| 3      | `EXPIRED`          | Expiration flag; writing 1 clears it          |
| 4      | `INTERRUPT_ENABLE` | Enables/disables IRQ generation on expiration |

`PERIOD = N` causes the Timer to expire after `N` enabled timer ticks. A period of `0` leaves the Timer idle.

Timer register access and interrupt signaling use separate hardware paths:

```text
Timer registers <-> Bus <-> CPU
       |
       +-- IRQ --> InterruptController --> CPU
```

When `EXPIRED` is active and interrupts are enabled, the Timer asserts its configured interrupt line.

Peripheral interrupt conditions are level-sensitive. CPU acknowledgement of an interrupt does not automatically clear the condition that caused the peripheral to assert its interrupt line.

For example, clearing the interrupt controller's pending state does not clear a Timer's `EXPIRED` condition or an ADC's `COMPLETE` condition. Firmware must clear the appropriate peripheral status flag through its memory-mapped register interface.

`InterruptController` tracks interrupt requests without needing to know which peripheral generated them.

The CPU checks for pending interrupts between instructions. When an interrupt is accepted, the CPU saves its return program counter, reads the handler address from its memory-based interrupt vector table, and begins executing the handler.

SimpleISA's `RETI` instruction restores the saved return address and resumes interrupted firmware.

A halted CPU can be awakened by a pending interrupt. Peripherals continue advancing while the CPU is halted.

The current SimpleCPU interrupt implementation is intentionally basic:

* Interrupts do not nest.
* Lower-numbered pending IRQs are selected first.
* Only the return program counter is automatically preserved.
* Firmware is responsible for clearing peripheral interrupt conditions.

## RAM and Memory Map

RAM is currently **32-bit word-addressed**.

For example:

```cpp
BoardConfig config;
config.ramWords = 1024;
```

configures 1,024 addressable 32-bit RAM words, representing 4,096 bytes of storage capacity.

Memory-mapped peripherals share the system bus with RAM. Board configuration determines the base addresses used by RAM, GPIO, Timers, and ADCs.

The default configuration places RAM at address `0`, allowing SimpleCPU to begin executing loaded firmware immediately after reset.

## Configurable Boards

`BoardConfig` provides configuration for major board characteristics, including:

* CPU clock frequency
* CPU register count
* RAM size
* RAM base address
* GPIO pin count
* GPIO base address
* Logic voltage
* Digital HIGH threshold
* Interrupt count
* Timer instances and their base addresses/interrupt numbers
* ADC instances and their base addresses/interrupt numbers
* ADC channel counts
* ADC channel-to-pin mappings
* ADC resolution
* ADC reference voltage
* ADC conversion timing

Invalid board configurations are rejected when the simulated board is created rather than being treated as runtime firmware errors.

The goal is to let users create generic board configurations approximating the resources and performance characteristics needed for embedded projects without requiring boardAF to reproduce a particular commercial MCU exactly.

## Example: ADC/GPIO Interrupt Firmware

v1 includes a standalone example:

```text
examples/adc_gpio_interrupt_demo.cpp
```

The example demonstrates a complete external-world-to-firmware-to-external-world path:

```text
External analog voltage
        |
        v
    GPIO pin 0
        |
        v
       ADC
        |
        v
 ADC interrupt
        |
        v
 SimpleCPU wakes
        |
        v
    ADC ISR
        |
        v
 Firmware decision
        |
        v
    GPIO pin 1
        |
        v
 External observable output
```

The example loads a machine-code firmware image, applies an external voltage to an analog input, performs an ADC conversion, wakes the halted CPU through an ADC interrupt, executes an interrupt service routine, clears the ADC interrupt condition, returns with `RETI`, and exposes the firmware's decision through a GPIO output.

This demonstrates the primary boardAF design goal: the external environment provides signals while firmware running inside the simulated controller determines how the controller responds.

## Building

boardAF uses CMake and requires a C++20-compatible compiler.

From the repository root:

```sh
cmake -S . -B build
cmake --build build
```

On systems where a specific CMake generator is required, select the appropriate generator when configuring the build.

The main executable is built as:

```text
boardAF
```

The ADC/GPIO demonstration is built as:

```text
adc_gpio_interrupt_demo
```

## Running the Example

After building, run the `adc_gpio_interrupt_demo` executable.

For example, on Windows from the build directory:

```cmd
adc_gpio_interrupt_demo.exe
```

A successful run reports the supplied input voltage, resulting ADC value, GPIO output state, and final CPU halt state.

## Running Tests

After building:

```sh
ctest --test-dir build --output-on-failure
```

The test suite covers major boardAF subsystems, including:

* RAM
* Bus
* GPIO
* Timer
* ADC
* CPU
* Simulation behavior
* Interrupt controller
* External pin access
* Real-time execution state
* Firmware loading
* Timer interrupt behavior
* ADC interrupt behavior
* CPU HALT/interrupt wake-up
* End-to-end firmware execution
* CAN subsystem components retained in the source tree

## Project Structure

```text
boardAF/
├── docs/       Project design and architecture documentation
├── examples/   Standalone boardAF usage examples
├── include/    Public headers
├── src/        Implementation source
├── tests/      Automated tests
├── CMakeLists.txt
└── README.md
```

## v1.0.0 Limitations

boardAF v1.0.0 intentionally focuses on proving the core architecture.

Current limitations include:

* SimpleISA uses 16-bit direct address/operand fields.
* SimpleISA provides only a small instruction set.
* There is no stack or `CALL`/`RET` instruction support.
* There is no indirect memory addressing.
* Comparison and conditional branching are currently limited.
* CPU interrupts do not nest.
* Only the interrupt return address is automatically preserved.
* The current CPU architecture uses 32-bit registers and machine words.
* RAM is word-addressed rather than byte-addressed.
* UART, SPI, I²C, PWM, and similar peripherals are not implemented.
* CAN-related components exist in the source tree, but CAN/multi-controller integration is not part of the supported v1 board workflow.
* Assemblers, compilers, linkers, debuggers, and executable file formats are not provided.
* boardAF does not attempt exact emulation of AVR, STM32, PIC, or other commercial MCU families.
* Detailed electrical and physical-world simulation is outside boardAF's scope.

These limitations are deliberate boundaries for the first release rather than requirements for the core proof of concept.

## Future Direction

Possible future development includes:

* Additional CPU instructions and addressing modes
* Additional CPU architectures or instruction sets
* More advanced interrupt behavior
* PWM and additional pin functions
* UART, SPI, I²C, and other peripherals
* CAN and multi-controller communication
* More flexible board profiles and peripheral layouts
* External assembler/compiler tooling
* Debugging and firmware-development tools
* DMA and multiple bus masters where justified
* Refinement of the external integration API as additional hardware interfaces are added
* Further architectural cleanup and optimization based on real boardAF use cases

These items describe possible project direction and should not be assumed to be implemented unless documented otherwise.

## Scope

boardAF focuses on the simulated embedded controller.

Detailed physical-world simulation, vehicle dynamics, robotics physics, circuit simulation, sensor physics, and similar environment-specific behavior are outside the core scope of the project.

Those systems can instead interact with boardAF through its external interfaces, allowing the simulated controller to treat its environment as the real world while boardAF remains focused on the microcontroller itself.

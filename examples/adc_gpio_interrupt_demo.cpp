#include <cstdint>
#include <iostream>
#include <vector>

#include "Simulation/Simulation.h"
#include "CPU/SimpleCPU/SimpleISA.h"

int main ()
{
	// Create the firmware image.
	// Unused addresses are filled with NOP instructions.
	// The image is large enough to contain:
	//   - the main program at 0x0000
	//   - the interrupt vector table beginning at 0x0100
	//   - the ADC interrupt service routine beginning at 0x0120
	std::vector<std::uint32_t> firmware(
			0x012B,
			SimpleISA::encode(SimpleISA::Opcode::NOP)
		);
		
	// -----------------------------------------------------------------
	// Main program
	//
	// Configure GPIO pin 1 as an output.
	// GPIO register map:
	//   0x1000 = PIN_SELECT
	//   0x1001 = DIRECTION
	//   0x1002 = OUTPUT
	//   0x1003 = INPUT
	// -----------------------------------------------------------------

	// Select GPIO pin 1.
	firmware[0x0000] =
		SimpleISA::encode(
			SimpleISA::Opcode::MOVI,
			0, 0, 1
		);

	firmware[0x0001] =
		SimpleISA::encode(
			SimpleISA::Opcode::STORE,
			0, 0, 0x1000
		);

	// Set the selected GPIO pin to OUTPUT mode.
	firmware[0x0002] =
		SimpleISA::encode(
			SimpleISA::Opcode::MOVI,
			0, 0, 1
		);

	firmware[0x0003] =
		SimpleISA::encode(
			SimpleISA::Opcode::STORE,
			0, 0, 0x1001
		);

	// -----------------------------------------------------------------
	// Configure the ADC.
	//
	// ADC register map:
	//   0x3000 = CHANNEL
	//   0x3001 = CONTROL
	//   0x3002 = STATUS
	//   0x3003 = RESULT
	//   0x3004 = RESULT_CHANNEL
	//   0x3005 = INTERRUPT_ENABLE
	// -----------------------------------------------------------------

	// Select ADC channel 0.
	firmware[0x0004] =
		SimpleISA::encode(
			SimpleISA::Opcode::MOVI,
			0, 0, 0
		);

	firmware[0x0005] =
		SimpleISA::encode(
			SimpleISA::Opcode::STORE,
			0, 0, 0x3000
		);

	// Enable the ADC interrupt.
	firmware[0x0006] =
		SimpleISA::encode(
			SimpleISA::Opcode::MOVI,
			0, 0, 1
		);

	firmware[0x0007] =
		SimpleISA::encode(
			SimpleISA::Opcode::STORE,
			0, 0, 0x3005
		);

	// Start an ADC conversion.
	firmware[0x0008] =
		SimpleISA::encode(
			SimpleISA::Opcode::MOVI,
			0, 0, 1
		);

	firmware[0x0009] =
		SimpleISA::encode(
			SimpleISA::Opcode::STORE,
			0, 0, 0x3001
		);

	// Halt the CPU while waiting for the ADC interrupt.
	// Peripherals continue running while the CPU is halted.
	firmware[0x000A] =
		SimpleISA::encode(
			SimpleISA::Opcode::HALT
		);

	// After the interrupt handler executes RETI, execution resumes here.
	// Jump back to HALT so the CPU returns to its idle state.
	firmware[0x000B] =
		SimpleISA::encode(
			SimpleISA::Opcode::JMP,
			0, 0, 0x000A
		);
			
	// -----------------------------------------------------------------
	// Interrupt vector table
	//
	// Interrupt vectors begin at 0x0100.
	// The ADC uses interrupt number 2, so its vector is stored at 0x0102.
	// The vector contains the ADDRESS of the ISR, not an instruction.
	// -----------------------------------------------------------------

	firmware[0x0102] = 0x0120;
		
	// -----------------------------------------------------------------
	// ADC interrupt service routine
	//
	// The ADC ISR begins at address 0x0120.
	//
	// R0 is used as a temporary register for peripheral writes.
	// R1 holds the ADC conversion result.
	// -----------------------------------------------------------------

	// Read the completed ADC conversion result into R1.
	firmware[0x0120] =
		SimpleISA::encode(
			SimpleISA::Opcode::LOAD,
			1, 0, 0x3003
		);

	// Compare the ADC result with 818.
	// With the default 10-bit ADC and 5.0 V reference,
	// an input of 4.0 V converts to approximately 818.
	firmware[0x0121] =
		SimpleISA::encode(
			SimpleISA::Opcode::CMPI,
			1, 0, 818
		);

	// If the result equals 818, jump to the HIGH-output path.
	firmware[0x0122] =
		SimpleISA::encode(
			SimpleISA::Opcode::JZ,
			0, 0, 0x0126
		);

	// -----------------------------------------------------------------
	// LOW path
	//
	// The ADC result did not equal 818, so drive GPIO pin 1 LOW.
	// -----------------------------------------------------------------

	firmware[0x0123] =
		SimpleISA::encode(
			SimpleISA::Opcode::MOVI,
			0, 0, 0
		);

	firmware[0x0124] =
		SimpleISA::encode(
			SimpleISA::Opcode::STORE,
			0, 0, 0x1002
		);

	// Skip over the HIGH path and continue to interrupt cleanup.
	firmware[0x0125] =
		SimpleISA::encode(
			SimpleISA::Opcode::JMP,
			0, 0, 0x0128
		);

	// -----------------------------------------------------------------
	// HIGH path
	//
	// The ADC result matched 818, so drive GPIO pin 1 HIGH.
	// -----------------------------------------------------------------

	firmware[0x0126] =
		SimpleISA::encode(
			SimpleISA::Opcode::MOVI,
			0, 0, 1
		);

	firmware[0x0127] =
		SimpleISA::encode(
			SimpleISA::Opcode::STORE,
			0, 0, 0x1002
		);

	// -----------------------------------------------------------------
	// Interrupt cleanup
	//
	// STATUS bit 1 is the ADC COMPLETE flag.
	// Writing a 1 to this bit clears the flag and therefore
	// deasserts the ADC interrupt line.
	// -----------------------------------------------------------------

	firmware[0x0128] =
		SimpleISA::encode(
			SimpleISA::Opcode::MOVI,
			0, 0, 2
		);

	firmware[0x0129] =
		SimpleISA::encode(
			SimpleISA::Opcode::STORE,
			0, 0, 0x3002
		);

	// Return from the interrupt.
	firmware[0x012A] =
		SimpleISA::encode(
			SimpleISA::Opcode::RETI
		);
			
	// -----------------------------------------------------------------
	// Create the simulated board and provide an external analog input.
	//
	// Pin 0 is connected to ADC channel 0 by the default board
	// configuration. From the firmware's point of view, this is simply
	// a voltage present on a physical MCU pin.
	// -----------------------------------------------------------------

	Simulator simulator;

	simulator.setPinVoltage(0, 1.0);

	// Load the machine-code firmware into the board's RAM.
	simulator.loadFirmware(firmware);

	// Run enough MCU clock cycles for:
	//   - initialization
	//   - ADC conversion
	//   - interrupt handling
	//   - return to HALT
	simulator.run(50);
		
	// -----------------------------------------------------------------
	// Display the externally observable result of the simulation.
	// -----------------------------------------------------------------

	std::cout << "boardAF ADC/GPIO interrupt demo\n\n";

	std::cout << "Input voltage: 1.0 V\n";

	std::cout
		<< "ADC result: "
		<< simulator.getADC().read(3)
		<< '\n';

	std::cout
		<< "GPIO pin 1: "
		<< (simulator.getGPIO().getPin(1).getOutputLatch()
			? "HIGH"
			: "LOW")
		<< '\n';

	std::cout
		<< "CPU halted: "
		<< (simulator.getCPU().isHalted()
			? "yes"
			: "no")
		<< '\n';
		
	return 0;
}
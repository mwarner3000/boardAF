#include <cassert>
#include <iostream>
#include <stdexcept>
#include <cstddef>

#include "CPU/SimpleCPU.h"
#include "Bus/Bus.h"
#include "Memory/RAM.h"
#include "Devices/GPIO.h"
#include "Simulator/Simulator.h"
#include "CPU/SimpleCPU/SimpleISA.h"
#include "Interrupts/InterruptController.h"

int main()
{
    // Reset state.
    {
        Bus bus;
        RAM ram(256);

        bus.attach(ram, 0x0000, 0x00FF);

        InterruptController interruptController;
		SimpleCPU cpu(bus, interruptController);

        cpu.reset();

        assert(cpu.getProgramCounter() == 0);
        assert(cpu.getRegister(0) == 0);
        assert(!cpu.isHalted());
    }

    // NOP should advance the program counter.
    {
        Bus bus;
        RAM ram(256);

        bus.attach(ram, 0x0000, 0x00FF);

        ram.write(0, 0x00000000);

        InterruptController interruptController;
		SimpleCPU cpu(bus, interruptController);

        cpu.reset();
        cpu.tick(1);

        assert(cpu.getProgramCounter() == 1);
        assert(cpu.getRegister(0) == 0);
        assert(!cpu.isHalted());
    }
	
    // ADD immediate.
    {
        Bus bus;
        RAM ram(256);

        bus.attach(ram, 0x0000, 0x00FF);
		
		ram.write(
			0,
			SimpleISA::encode(
				SimpleISA::Opcode::MOVI,
				1,
				0,
				5
			)
		);
		
		ram.write(
			1,
			SimpleISA::encode(
				SimpleISA::Opcode::ADD,
				0,
				1,
				0
			)
		);

        InterruptController interruptController;
		SimpleCPU cpu(bus, interruptController);

        cpu.reset();
        cpu.tick(1);
        cpu.tick(2);

        assert(cpu.getRegister(0) == 5);
        assert(cpu.getProgramCounter() == 2);
    }
	
    // LOAD from memory.
    {
        Bus bus;
        RAM ram(256);

        bus.attach(ram, 0x0000, 0x00FF);

        ram.write(0, 0x1100000A);
        ram.write(10, 123);

        InterruptController interruptController;
		SimpleCPU cpu(bus, interruptController);

        cpu.reset();
        cpu.tick(1);

        assert(cpu.getRegister(0) == 123);
    }
	
    // STORE to memory.
    {
        Bus bus;
        RAM ram(256);

        bus.attach(ram, 0x0000, 0x00FF);

        ram.write(
			0,
			SimpleISA::encode(
				SimpleISA::Opcode::MOVI,
				0,
				0,
				42
			)
		);

		ram.write(
			1,
			SimpleISA::encode(
				SimpleISA::Opcode::STORE,
				0,
				0,
				10
			)
		);

        InterruptController interruptController;
		SimpleCPU cpu(bus, interruptController);

        cpu.reset();

        cpu.tick(1);
        cpu.tick(2);

        assert(ram.read(10) == 42);
    }

    // HALT.
    {
        Bus bus;
        RAM ram(256);

        bus.attach(ram, 0x0000, 0x00FF);

        ram.write(0, 0xFF000000);

        InterruptController interruptController;
		SimpleCPU cpu(bus, interruptController);

        cpu.reset();
        cpu.tick(1);

        assert(cpu.isHalted());
        assert(cpu.getProgramCounter() == 1);

        // Additional ticks should do nothing.
        cpu.tick(2);

        assert(cpu.getProgramCounter() == 1);
    }

    // Invalid opcode should throw.
    {
        Bus bus;
        RAM ram(256);

        bus.attach(ram, 0x0000, 0x00FF);

        ram.write(0, 0xAA000000);

        InterruptController interruptController;
		SimpleCPU cpu(bus, interruptController);

        cpu.reset();

        bool exceptionThrown = false;

        try
        {
            cpu.tick(1);
        }
        catch (const std::runtime_error&)
        {
            exceptionThrown = true;
        }

        assert(exceptionThrown);
    }
	
	//reset test for all eight registers
	{
		Bus bus;
		RAM ram(256);

		bus.attach(ram, 0x0000, 0x00FF);

		InterruptController interruptController;
		SimpleCPU cpu(bus, interruptController);

		cpu.reset();

		for (std::size_t i = 0;
			 i < 8;
			 ++i)
		{
			assert(cpu.getRegister(i) == 0);
		}
	}
	
	//test for invalid registers
	{
		Bus bus;
		RAM ram(256);

		bus.attach(ram, 0x0000, 0x00FF);

		InterruptController interruptController;
		SimpleCPU cpu(bus, interruptController);

		bool exceptionThrown = false;

		try
		{
			cpu.getRegister(8);
		}
		catch (const std::out_of_range&)
		{
			exceptionThrown = true;
		}

		assert(exceptionThrown);
	}

    // Full program:
    //
    // LOAD R0, [10]
    // ADD  R0, 5
    // STORE R0, [11]
    // HALT
    {
        Simulator simulator;

        Bus& bus = simulator.getBus();
        SimpleCPU& cpu = simulator.getCPU();

        bus.write(0, 0x1100000A);
		bus.write(1, 0x10100005);
        bus.write(2, 0x14010000);
        bus.write(3, 0x1200000B);
        bus.write(4, 0xFF000000);

        bus.write(10, 20);
        bus.write(11, 0);

        cpu.reset();

        RunResult result = simulator.run(100);

        assert(result == RunResult::CycleLimitReached);
        assert(cpu.getRegister(0) == 25);
        assert(bus.read(11) == 25);
        assert(simulator.getClock().getCycle() == 100);
    }

    // Firmware-driven GPIO.
    {
		Simulator simulator;

		Bus& bus = simulator.getBus();
		SimpleCPU& cpu = simulator.getCPU();
		GPIO& gpio = simulator.getGPIO();

		// R0 = 7
		bus.write(
			0,
			SimpleISA::encode(
				SimpleISA::Opcode::MOVI,
				0,
				0,
				7
			)
		);

		// GPIO PIN_SELECT = 7
		bus.write(
			1,
			SimpleISA::encode(
				SimpleISA::Opcode::STORE,
				0,
				0,
				0x1000
			)
		);

		// R0 = 1
		bus.write(
			2,
			SimpleISA::encode(
				SimpleISA::Opcode::MOVI,
				0,
				0,
				1
			)
		);

		// GPIO DIRECTION = Output
		bus.write(
			3,
			SimpleISA::encode(
				SimpleISA::Opcode::STORE,
				0,
				0,
				0x1001
			)
		);

		// GPIO OUTPUT = HIGH
		bus.write(
			4,
			SimpleISA::encode(
				SimpleISA::Opcode::STORE,
				0,
				0,
				0x1002
			)
		);

		// HALT
		bus.write(
			5,
			SimpleISA::encode(
				SimpleISA::Opcode::HALT
			)
		);

		cpu.reset();

		RunResult result =
			simulator.run(100);

		assert(result == RunResult::CycleLimitReached);

		assert(bus.read(0x1000) == 7);
		assert(bus.read(0x1001) == 1);
		assert(bus.read(0x1002) == 1);

		assert(
			gpio.getPin(7).getDirection() ==
			PinDirection::Output
		);

		assert(
			gpio.getPin(7)
				.getEffectiveVoltage(5.0)
				.value() == 5.0
		);
	}

    // Cycle limit should stop runaway firmware.
    {
        Simulator simulator;

        Bus& bus = simulator.getBus();
        SimpleCPU& cpu = simulator.getCPU();

        // RAM defaults to zero, so this is effectively
        // an endless stream of NOP instructions.
        cpu.reset();

        RunResult result = simulator.run(10);

        assert(result == RunResult::CycleLimitReached);
        assert(simulator.getClock().getCycle() == 10);
        assert(!cpu.isHalted());
    }
	
	//register aware program test
	{
		Simulator simulator;

		Bus& bus = simulator.getBus();
		SimpleCPU& cpu = simulator.getCPU();

		// MOV R1, 10
		bus.write(0, 0x1010000A);

		// MOV R2, 20
		bus.write(1, 0x10200014);

		// ADD R1, R2
		bus.write(2, 0x14120000);

		// MOV R3, R1
		bus.write(3, 0x13310000);

		// HALT
		bus.write(4, 0xFF000000);

		cpu.reset();

		RunResult result =
			simulator.run(100);

		assert(result == RunResult::CycleLimitReached);

		assert(cpu.getRegister(1) == 30);
		assert(cpu.getRegister(2) == 20);
		assert(cpu.getRegister(3) == 30);

		assert(simulator.getClock().getCycle() == 100);
	}
	
	//register aware memory operations
	{
		Simulator simulator;

		Bus& bus = simulator.getBus();
		SimpleCPU& cpu = simulator.getCPU();

		// MOV R4, 123
		bus.write(0, 0x1040007B);

		// STORE R4, [100]
		bus.write(1, 0x12400064);

		// LOAD R5, [100]
		bus.write(2, 0x11500064);

		// HALT
		bus.write(3, 0xFF000000);

		cpu.reset();

		RunResult result =
			simulator.run(100);

		assert(result == RunResult::CycleLimitReached);

		assert(bus.read(100) == 123);
		assert(cpu.getRegister(4) == 123);
		assert(cpu.getRegister(5) == 123);
	}
	
	//invalid register encoding
	{
		Bus bus;
		RAM ram(256);

		bus.attach(ram, 0x0000, 0x00FF);

		// MOV R8, 123 -- invalid
		ram.write(0, 0x1080007B);

		InterruptController interruptController;
		SimpleCPU cpu(bus, interruptController);

		cpu.reset();

		bool exceptionThrown = false;

		try
		{
			cpu.tick(1);
		}
		catch (const std::runtime_error&)
		{
			exceptionThrown = true;
		}

		assert(exceptionThrown);
	}
	
	//CMP test
	{
		Simulator simulator;

		Bus& bus = simulator.getBus();
		SimpleCPU& cpu = simulator.getCPU();

		// MOV R1, 10
		bus.write(0, 0x1010000A);

		// MOV R2, 10
		bus.write(1, 0x1020000A);

		// CMP R1, R2
		bus.write(2, 0x20120000);

		// HALT
		bus.write(3, 0xFF000000);

		cpu.reset();

		RunResult result =
			simulator.run(100);

		assert(result == RunResult::CycleLimitReached);
		assert(cpu.getZeroFlag());
	}
	
	//test unequal values
	{
		Simulator simulator;

		Bus& bus = simulator.getBus();
		SimpleCPU& cpu = simulator.getCPU();

		bus.write(0, 0x1010000A); // MOV R1, 10
		bus.write(1, 0x10200014); // MOV R2, 20
		bus.write(2, 0x20120000); // CMP R1, R2
		bus.write(3, 0xFF000000); // HALT

		cpu.reset();

		simulator.run(100);

		assert(!cpu.getZeroFlag());
	}
	
	//test conditional branching
	{
		Simulator simulator;

		Bus& bus = simulator.getBus();
		SimpleCPU& cpu = simulator.getCPU();

		// MOV R1, 5
		bus.write(0, 0x10100005);

		// MOV R2, 5
		bus.write(1, 0x10200005);

		// CMP R1, R2
		bus.write(2, 0x20120000);

		// JZ 5
		bus.write(3, 0x22000005);

		// MOV R3, 111
		// Should be skipped.
		bus.write(4, 0x1030006F);

		// MOV R3, 222
		bus.write(5, 0x103000DE);

		// HALT
		bus.write(6, 0xFF000000);

		cpu.reset();

		RunResult result =
			simulator.run(100);

		assert(result == RunResult::CycleLimitReached);
		assert(cpu.getRegister(3) == 222);
	}
	
	//test JNZ
	{
		Simulator simulator;

		Bus& bus = simulator.getBus();
		SimpleCPU& cpu = simulator.getCPU();

		bus.write(
			0,
			SimpleISA::encode(
				SimpleISA::Opcode::MOVI,
				1,
				0,
				5
			)
		); // MOV R1, 5
		bus.write(1, 0x10200006); // MOV R2, 6
		bus.write(2, 0x20120000); // CMP R1, R2

		bus.write(3, 0x23000005); // JNZ 5

		bus.write(4, 0x1030006F); // MOV R3, 111
		bus.write(5, 0x103000DE); // MOV R3, 222

		bus.write(6, 0xFF000000);

		cpu.reset();

		simulator.run(100);

		assert(cpu.getRegister(3) == 222);
	}
		
	//subtraction test
	{
		Simulator simulator;

		Bus& bus = simulator.getBus();
		SimpleCPU& cpu = simulator.getCPU();

		// MOV R1, 20
		bus.write(0, 0x10100014);

		// MOV R2, 5
		bus.write(1, 0x10200005);

		// SUB R1, R2
		bus.write(2, 0x15120000);

		// HALT
		bus.write(3, 0xFF000000);

		cpu.reset();

		RunResult result =
			simulator.run(100);

		assert(result == RunResult::CycleLimitReached);
		assert(cpu.getRegister(1) == 15);
	}

	//CMPI test
	{
		Simulator simulator;

		Bus& bus = simulator.getBus();
		SimpleCPU& cpu = simulator.getCPU();

		// MOV R1, 10
		bus.write(0, 0x1010000A);

		// CMPI R1, 10
		bus.write(1, 0x2410000A);

		// HALT
		bus.write(2, 0xFF000000);

		cpu.reset();

		simulator.run(100);

		assert(cpu.getZeroFlag());
	}

	//unequal case test
	{
		Simulator simulator;

		Bus& bus = simulator.getBus();
		SimpleCPU& cpu = simulator.getCPU();

		bus.write(0, 0x1010000A); // MOV R1, 10
		bus.write(1, 0x24100005); // CMPI R1, 5
		bus.write(2, 0xFF000000); // HALT

		cpu.reset();

		simulator.run(100);

		assert(!cpu.getZeroFlag());
	}
		
	//loop test
	{
		Simulator simulator;

		Bus& bus = simulator.getBus();
		SimpleCPU& cpu = simulator.getCPU();

		// 0: MOV R1, 5
		bus.write(0, 0x10100005);

		// 1: MOV R2, 1
		bus.write(1, 0x10200001);

		// 2: SUB R1, R2
		bus.write(2, 0x15120000);

		// 3: CMPI R1, 0
		bus.write(3, 0x24100000);

		// 4: JNZ 2
		bus.write(4, 0x23000002);

		// 5: HALT
		bus.write(5, 0xFF000000);

		cpu.reset();

		RunResult result =
			simulator.run(100);

		assert(result == RunResult::CycleLimitReached);
		assert(cpu.getRegister(1) == 0);
		assert(cpu.getZeroFlag());
	}
	
	// CPU should service an interrupt and return to
	// the interrupted program.
	{
		Bus bus;
		RAM ram(512);
		InterruptController interruptController;

		bus.attach(ram, 0, 511);

		SimpleCPU cpu(
			bus,
			interruptController
		);

		// Normal firmware:
		//
		// 0: R0 = 10
		// 1: R1 = 20
		// 2: HALT

		bus.write(
			0,
			SimpleISA::encode(
				SimpleISA::Opcode::MOVI,
				0,
				0,
				10
			)
		);

		bus.write(
			1,
			SimpleISA::encode(
				SimpleISA::Opcode::MOVI,
				1,
				0,
				20
			)
		);

		bus.write(
			2,
			SimpleISA::encode(
				SimpleISA::Opcode::HALT
			)
		);

		// IRQ 3 vector points to address 100.
		bus.write(
			SimpleCPU::InterruptVectorBase + 3,
			100
		);

		// Interrupt handler:
		//
		// 100: R2 = 99
		// 101: RETI

		bus.write(
			100,
			SimpleISA::encode(
				SimpleISA::Opcode::MOVI,
				2,
				0,
				99
			)
		);

		bus.write(
			101,
			SimpleISA::encode(
				SimpleISA::Opcode::RETI
			)
		);

		cpu.reset();

		// Execute normal instruction at address 0.
		cpu.tick(1);

		assert(cpu.getRegister(0) == 10);
		assert(cpu.getProgramCounter() == 1);

		// Hardware requests IRQ 3 between instructions.
		interruptController.request(3);

		// CPU should enter the handler and execute
		// its first instruction.
		cpu.tick(2);

		assert(cpu.getRegister(2) == 99);
		assert(cpu.getProgramCounter() == 101);

		// Execute RETI.
		cpu.tick(3);

		// We should return to address 1, which had
		// not executed before the interrupt.
		assert(cpu.getProgramCounter() == 1);

		// Resume normal firmware.
		cpu.tick(4);

		assert(cpu.getRegister(1) == 20);
		assert(cpu.getProgramCounter() == 2);

		// HALT.
		cpu.tick(5);

		assert(cpu.isHalted());
	}
	
	//proves a halted CPU wakes for an interrupt
	{
		Bus bus;
		RAM ram(512);
		InterruptController interruptController;

		bus.attach(ram, 0, 511);

		SimpleCPU cpu(
			bus,
			interruptController
		);
		
		bus.write(
			0,
			SimpleISA::encode(
				SimpleISA::Opcode::HALT
			)
		);
		
		bus.write(
			SimpleCPU::InterruptVectorBase + 0,
			100
		);
		
		bus.write(
			100,
			SimpleISA::encode(
				SimpleISA::Opcode::MOVI,
				1,
				0,
				42
			)
		);
		
		bus.write(
			101,
			SimpleISA::encode(
				SimpleISA::Opcode::RETI
			)
		);
		
		cpu.reset();
		cpu.tick(1);
		
		assert(cpu.isHalted());
		
		interruptController.request(0);
		
		cpu.tick(2);
		
		assert(!cpu.isHalted());
		assert(cpu.getRegister(1) == 42);
		
	}
	
    std::cout << "CPU tests passed.\n";

    return 0;
}


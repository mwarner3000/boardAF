#include "CPU/SimpleCPU.h"
#include "CPU/SimpleCPU/SimpleISA.h"

#include <stdexcept>
#include <algorithm>

SimpleCPU::SimpleCPU(
    Bus& bus,
    InterruptController& interruptController,
    std::size_t registerCount
)
    : bus(bus),
      interruptController(interruptController),
      programCounter(0),
      registers(registerCount, 0),
      halted(false),
      zeroFlag(false),
	  interruptReturnAddress(0),
	  servicingInterrupt(false)
{
	if (registerCount == 0 || registerCount > 16)
	{
		throw std::invalid_argument(
			"CPU register count must be between 1 and 16"
		);
	}
}

void SimpleCPU::reset()
{
    programCounter = 0;
    std::fill(
		registers.begin(),
		registers.end(),
		0
	);
    halted = false;
    zeroFlag = false;
	interruptReturnAddress = 0;
	servicingInterrupt = false;
}

void SimpleCPU::tick(std::uint64_t /*cycle*/)
{
	if (!servicingInterrupt &&
		interruptController.hasPending())
	{
		const std::size_t interruptNumber =
			interruptController.getNextPending();

		interruptReturnAddress = programCounter;

		const std::uint32_t vectorAddress =
			InterruptVectorBase +
			static_cast<std::uint32_t>(
				interruptNumber
			);

		programCounter =
			bus.read(vectorAddress);

		interruptController.clear(
			interruptNumber
		);

		servicingInterrupt = true;
		halted = false;
	}
	
    if (halted)
    {
        return;
    }
	

    std::uint32_t rawInstruction =
        bus.read(programCounter);

    SimpleISA::Instruction instruction =
        SimpleISA::decode(rawInstruction);

    const std::uint8_t rd = instruction.rd;
    const std::uint8_t rs = instruction.rs;
    const std::uint16_t operand = instruction.operand;

    ++programCounter;

    switch (instruction.opcode)
	{
        case SimpleISA::Opcode::NOP:
			break;
			
		case SimpleISA::Opcode::MOVI:
			// MOV Rd, immediate

			if (rd >= registers.size())
			{
				throw std::runtime_error(
					"Invalid destination register"
				);
			}

			registers[rd] = operand;
			break;

		case SimpleISA::Opcode::LOAD:
			// LOAD Rd, [address]

			if (rd >= registers.size())
			{
				throw std::runtime_error(
					"Invalid destination register"
				);
			}

			registers[rd] = bus.read(operand);
			break;

		case SimpleISA::Opcode::STORE:
			// STORE Rd, [address]

			if (rd >= registers.size())
			{
				throw std::runtime_error(
					"Invalid source register"
				);
			}

			bus.write(operand, registers[rd]);
			break;

		case SimpleISA::Opcode::MOV:	
			// MOV Rd, Rs

			if (rd >= registers.size() ||
				rs >= registers.size())
			{
				throw std::runtime_error(
					"Invalid register"
				);
			}

			registers[rd] = registers[rs];
			break;

		case SimpleISA::Opcode::ADD:
			// ADD Rd, Rs

			if (rd >= registers.size() ||
				rs >= registers.size())
			{
				throw std::runtime_error(
					"Invalid register"
				);
			}

			registers[rd] += registers[rs];
			break;
			
		case SimpleISA::Opcode::SUB:
			// SUB Rd, Rs

			if (rd >= registers.size() ||
				rs >= registers.size())
			{
				throw std::runtime_error(
					"Invalid register"
				);
			}

			registers[rd] -= registers[rs];
			break;
			
		case SimpleISA::Opcode::CMP:
			// CMP Rd, Rs

			if (rd >= registers.size() ||
				rs >= registers.size())
			{
				throw std::runtime_error(
					"Invalid register"
				);
			}

			zeroFlag =
				registers[rd] == registers[rs];

			break;

		case SimpleISA::Opcode::JMP:
			// JMP address
			programCounter = operand;
			break;

		case SimpleISA::Opcode::JZ:
			// JZ address
			if (zeroFlag)
			{
				programCounter = operand;
			}
			break;

		case SimpleISA::Opcode::JNZ:
			// JNZ address
			if (!zeroFlag)
			{
				programCounter = operand;
			}
			break;
			
		case SimpleISA::Opcode::CMPI:
			// CMPI Rd, immediate

			if (rd >= registers.size())
			{
				throw std::runtime_error(
					"Invalid register"
				);
			}

			zeroFlag =
				registers[rd] == operand;

			break;	
			
		case SimpleISA::Opcode::RETI:
			if (!servicingInterrupt)
			{
				throw std::runtime_error(
					"RETI executed outside interrupt handler"
				);
			}

			programCounter = interruptReturnAddress;
			servicingInterrupt = false;
			break;

        case SimpleISA::Opcode::HALT:
            // HALT
            halted = true;
            break;

        default:
            throw std::runtime_error(
                "SimpleCPU encountered invalid opcode"
            );
    }
}

std::uint32_t SimpleCPU::getProgramCounter() const
{
    return programCounter;
}

bool SimpleCPU::isHalted() const
{
    return halted;
}

std::uint32_t SimpleCPU::getRegister(std::size_t index) const
{
    if (index >= registers.size())
    {
        throw std::out_of_range(
            "Invalid CPU register index"
        );
    }

    return registers[index];
}

bool SimpleCPU::getZeroFlag() const
{
    return zeroFlag;
}
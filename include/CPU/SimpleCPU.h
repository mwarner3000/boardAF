#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "CPU/ICPU.h"
#include "Bus/Bus.h"
#include "Interrupts/InterruptController.h"

class SimpleCPU : public ICPU
{
public:
    std::vector<std::uint32_t> registers;

    SimpleCPU(
		Bus& bus,
		InterruptController& interruptController,
		std::size_t registerCount = 8
	);

    void reset() override;
    void tick(std::uint64_t cycle) override;

    std::uint32_t getProgramCounter() const override;

    std::uint32_t getRegister(std::size_t index) const;

    bool isHalted() const;
	bool getZeroFlag() const;
	
	static constexpr std::uint32_t InterruptVectorBase = 0x0100;

private:
    Bus& bus;
	InterruptController& interruptController;

    std::uint32_t programCounter;

    bool halted;
	bool zeroFlag;
	
	std::uint32_t interruptReturnAddress;
	bool servicingInterrupt;
};
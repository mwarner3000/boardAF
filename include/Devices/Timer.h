#pragma once

#include <cstdint>
#include <cstddef>

#include "Bus/IBusDevice.h"
#include "Simulator/IClockable.h"
#include "Interrupts/InterruptController.h"

class Timer : public IBusDevice, public IClockable
{
public:
    Timer(
		InterruptController& interruptController,
		std::size_t interruptNumber
	);

    std::uint32_t read(std::uint32_t address) override;

    void write(std::uint32_t address,
               std::uint32_t value) override;

    void tick(std::uint64_t cycle) override;
	
	static constexpr std::uint32_t RegisterCount = 5;

private:
    std::uint16_t counter;
    std::uint16_t period;

    bool enabled;
    bool expired;
	
	InterruptController& interruptController;
	std::size_t interruptNumber;

	bool interruptEnabled;
};
#include "Devices/Timer.h"

#include <stdexcept>

Timer::Timer(
    InterruptController& interruptController,
    std::size_t interruptNumber
)
    : interruptController(interruptController),
      interruptNumber(interruptNumber),
      counter(0),
      period(0),
      enabled(false),
      expired(false),
      interruptEnabled(false)
{
}

std::uint32_t Timer::read(std::uint32_t address)
{
    switch (address)
    {
        case 0:
            return counter;

        case 1:
            return period;

        case 2:
            return enabled ? 1 : 0;

        case 3:
            return expired ? 1 : 0;
			
		case 4:
			return interruptEnabled ? 1 : 0;

        default:
            throw std::out_of_range(
                "Invalid Timer register address"
            );
    }
}

void Timer::write(std::uint32_t address,
                  std::uint32_t value)
{
    switch (address)
    {
        case 0:
            counter =
                static_cast<std::uint16_t>(value);
            break;

        case 1:
            period =
                static_cast<std::uint16_t>(value);
            break;

        case 2:
            enabled = (value & 1u) != 0;

            if (!enabled)
            {
                expired = false;
            }

            break;

        case 3:
            // Writing 1 clears the expired flag.
            if ((value & 1u) != 0)
            {
                expired = false;
            }

            break;
			
		case 4:
			interruptEnabled =
				(value & 1u) != 0;
			break;

        default:
            throw std::out_of_range(
                "Invalid Timer register address"
            );
    }
}

void Timer::tick(std::uint64_t /*cycle*/)
{
    if (!enabled)
        return;

    ++counter;

    if (counter >= period)
    {
        counter = 0;
        expired = true;

        if (interruptEnabled)
            interruptController.request(interruptNumber);
    }
}
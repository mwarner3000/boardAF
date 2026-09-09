#include "Interrupts/InterruptController.h"

#include <stdexcept>

InterruptController::InterruptController(
    std::size_t interruptCount
)
    : pending(interruptCount, false),
      asserted(interruptCount, false)
{
}

void InterruptController::request(
    std::size_t interruptNumber
)
{
    if (interruptNumber >= pending.size())
    {
        throw std::out_of_range(
            "Invalid interrupt number"
        );
    }

    pending[interruptNumber] = true;
}

void InterruptController::clear(
    std::size_t interruptNumber
)
{
    if (interruptNumber >= pending.size())
    {
        throw std::out_of_range(
            "Invalid interrupt number"
        );
    }

    pending[interruptNumber] = false;
}

bool InterruptController::isPending(
    std::size_t interruptNumber
) const
{
    if (interruptNumber >= pending.size())
    {
        throw std::out_of_range(
            "Invalid interrupt number"
        );
    }

    return pending[interruptNumber] ||
			asserted[interruptNumber];
}

bool InterruptController::hasPending() const
{
    for (std::size_t i = 0; i < pending.size(); ++i)
	{
		if (pending[i] || asserted[i])
		{
			return true;
		}
	}

	return false;
}

std::size_t InterruptController::getNextPending() const
{
    for (std::size_t i = 0; i < pending.size(); ++i)
	{
		if (pending[i] || asserted[i])
		{
			return i;
		}
	}

    throw std::runtime_error(
        "No interrupt is pending"
    );
}

void InterruptController::setLine(
    std::size_t interruptNumber,
    bool value
)
{
    if (interruptNumber >= asserted.size())
    {
        throw std::out_of_range(
            "Invalid interrupt number"
        );
    }

    asserted[interruptNumber] = value;
}
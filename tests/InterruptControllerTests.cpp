#include <cassert>
#include <iostream>
#include <stdexcept>

#include "Interrupts/InterruptController.h"

int main()
{
    InterruptController controller(8);

    assert(!controller.hasPending());

    controller.request(3);

    assert(controller.hasPending());
    assert(controller.isPending(3));
    assert(controller.getNextPending() == 3);

    controller.request(1);

    // Lowest interrupt number wins for now.
    assert(controller.getNextPending() == 1);

    controller.clear(1);

    assert(!controller.isPending(1));
    assert(controller.getNextPending() == 3);

    controller.clear(3);

    assert(!controller.hasPending());

    bool exceptionThrown = false;

    try
    {
        controller.request(8);
    }
    catch (const std::out_of_range&)
    {
        exceptionThrown = true;
    }

    assert(exceptionThrown);
	
	{
		InterruptController controller(8);

		controller.setLine(3, true);

		assert(controller.hasPending());
		assert(controller.isPending(3));
		assert(controller.getNextPending() == 3);

		// CPU acknowledgement must NOT clear an asserted line.
		controller.clear(3);

		assert(controller.hasPending());
		assert(controller.isPending(3));
		assert(controller.getNextPending() == 3);

		// The peripheral deasserting the line clears the condition.
		controller.setLine(3, false);

		assert(!controller.hasPending());
		assert(!controller.isPending(3));
	}
	
	{
		InterruptController controller(8);

		controller.request(3);

		assert(controller.hasPending());
		assert(controller.isPending(3));

		controller.clear(3);

		assert(!controller.hasPending());
		assert(!controller.isPending(3));
	}
	
	{
		InterruptController controller(8);

		controller.request(3);
		controller.setLine(3, true);

		controller.clear(3);

		// The event was acknowledged, but the hardware line remains.
		assert(controller.isPending(3));

		controller.setLine(3, false);

		assert(!controller.isPending(3));
	}

    std::cout
        << "Interrupt controller tests passed.\n";

    return 0;
}

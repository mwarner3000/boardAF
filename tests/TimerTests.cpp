#include <cassert>
#include <iostream>
#include <stdexcept>

#include "Bus/Bus.h"
#include "Devices/Timer.h"
#include "Simulator/Simulator.h"

int main()
{
    // Initial state.
    {
        InterruptController interruptController;
		Timer timer(interruptController, 0);

        assert(timer.read(0) == 0);
        assert(timer.read(1) == 0);
        assert(timer.read(2) == 0);
        assert(timer.read(3) == 0);
    }

    // Register reads and writes.
    {
        InterruptController interruptController;
		Timer timer(interruptController, 0);

        timer.write(0, 3);
        timer.write(1, 10);
        timer.write(2, 1);

        assert(timer.read(0) == 3);
        assert(timer.read(1) == 10);
        assert(timer.read(2) == 1);
    }

    // Disabled timer should not advance.
    {
        InterruptController interruptController;
		Timer timer(interruptController, 0);

        timer.write(1, 5);

        timer.tick(1);
        timer.tick(2);
        timer.tick(3);

        assert(timer.read(0) == 0);
        assert(timer.read(3) == 0);
    }

    // Enabled timer should advance.
    {
        InterruptController interruptController;
		Timer timer(interruptController, 0);

        timer.write(1, 5);
        timer.write(2, 1);

        timer.tick(1);
        timer.tick(2);
        timer.tick(3);

        assert(timer.read(0) == 3);
    }

    // Timer should expire and reset.
    {
        InterruptController interruptController;
		Timer timer(interruptController, 0);

        timer.write(1, 3);
        timer.write(2, 1);

        timer.tick(1);
        timer.tick(2);
        timer.tick(3);

        assert(timer.read(0) == 0);
        assert(timer.read(3) == 1);
    }

    // Writing 1 to status should clear expiration.
    {
        InterruptController interruptController;
		Timer timer(interruptController, 0);

        timer.write(1, 1);
        timer.write(2, 1);

        timer.tick(1);

        assert(timer.read(3) == 1);

        timer.write(3, 1);

        assert(timer.read(3) == 0);
    }

    // Disabling the timer should clear expiration.
    {
        InterruptController interruptController;
		Timer timer(interruptController, 0);

        timer.write(1, 1);
        timer.write(2, 1);

        timer.tick(1);

        assert(timer.read(3) == 1);

        timer.write(2, 0);

        assert(timer.read(2) == 0);
        assert(timer.read(3) == 0);
    }

    // Invalid register reads should fail.
    {
        InterruptController interruptController;
		Timer timer(interruptController, 0);

        bool exceptionThrown = false;

		try
		{
			timer.read(5);
		}
		catch (const std::out_of_range&)
		{
			exceptionThrown = true;
		}

		assert(exceptionThrown);
    }

    // Invalid register writes should fail.
    {
        InterruptController interruptController;
		Timer timer(interruptController, 0);

        bool exceptionThrown = false;

        try
        {
            timer.write(5, 0);
        }
        catch (const std::out_of_range&)
        {
            exceptionThrown = true;
        }

        assert(exceptionThrown);
    }

    // Bus access should reach the timer correctly.
    {
        Bus bus;
        InterruptController interruptController;
		Timer timer(interruptController, 0);

        bus.attach(timer, 0x2000, 0x2003);

        bus.write(0x2001, 5);
        bus.write(0x2002, 1);

        assert(bus.read(0x2001) == 5);
        assert(bus.read(0x2002) == 1);
    }

    // Simulator ticks should advance the timer exactly once per cycle.
    {
        Simulator simulator;

        Bus& bus = simulator.getBus();

        bus.write(0x2001, 10);
        bus.write(0x2002, 1);

        simulator.tick();
        simulator.tick();
        simulator.tick();

        assert(simulator.getClock().getCycle() == 3);
        assert(bus.read(0x2000) == 3);
    }
	
	// Timer should request an interrupt when it
	// expires and interrupts are enabled.
	{
		InterruptController interruptController;

		Timer timer(
			interruptController,
			2
		);

		timer.write(1, 3); // period
		timer.write(2, 1); // timer enabled
		timer.write(4, 1); // interrupt enabled

		assert(!interruptController.hasPending());

		timer.tick(1);
		timer.tick(2);
		timer.tick(3);
		timer.tick(4);

		assert(timer.read(3) == 1);

		assert(
			interruptController.isPending(2)
		);
	}
	
	{
		InterruptController interruptController(8);
		Timer timer(interruptController, 0);

		timer.write(1, 0); // PERIOD
		timer.write(2, 1); // ENABLE

		timer.tick(1);
		timer.tick(2);
		timer.tick(3);

		assert(timer.read(0) == 0); // COUNTER
		assert(timer.read(3) == 0); // EXPIRED
	}
	
	{
		InterruptController controller(8);
		Timer timer(controller, 0);

		timer.write(1, 2); // PERIOD
		timer.write(2, 1); // ENABLE
		timer.write(4, 1); // INTERRUPT_ENABLE

		timer.tick(1);
		timer.tick(2);

		assert(timer.read(3) == 1);
		assert(controller.isPending(0));

		// Simulate CPU acknowledgement.
		controller.clear(0);

		// Timer is still asserting its IRQ.
		assert(controller.isPending(0));

		// Firmware clears EXPIRED.
		timer.write(3, 1);

		assert(timer.read(3) == 0);
		assert(!controller.isPending(0));
	}
	
	{
		InterruptController controller(8);
		Timer timer(controller, 0);

		timer.write(1, 2); // PERIOD
		timer.write(2, 1); // ENABLE

		// Interrupt remains disabled.
		timer.tick(1);
		timer.tick(2);

		assert(timer.read(3) == 1);
		assert(!controller.isPending(0));

		// Existing EXPIRED condition should now assert IRQ.
		timer.write(4, 1);

		assert(controller.isPending(0));

		// Disabling interrupt should immediately deassert it.
		timer.write(4, 0);

		assert(!controller.isPending(0));
	}

    std::cout << "Timer tests passed.\n";

    return 0;
}
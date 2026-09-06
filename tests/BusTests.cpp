#include <cassert>
#include <iostream>
#include <stdexcept>

#include "Bus/Bus.h"
#include "Memory/RAM.h"

int main()
{
    // Basic mapping and address translation.
    {
        Bus bus;
        RAM ram(256);

        bus.attach(ram, 0x1000, 0x10FF);

        bus.write(0x100A, 123);

        assert(bus.read(0x100A) == 123);
        assert(ram.read(10) == 123);
    }

    // Multiple devices.
    {
        Bus bus;

        RAM ram1(256);
        RAM ram2(256);

        bus.attach(ram1, 0x1000, 0x10FF);
        bus.attach(ram2, 0x2000, 0x20FF);

        bus.write(0x1000, 111);
        bus.write(0x2000, 222);

        assert(bus.read(0x1000) == 111);
        assert(bus.read(0x2000) == 222);
    }

    // Adjacent mappings should be valid.
    {
        Bus bus;

        RAM ram1(256);
        RAM ram2(256);

        bus.attach(ram1, 0x1000, 0x10FF);
        bus.attach(ram2, 0x1100, 0x11FF);

        bus.write(0x10FF, 111);
        bus.write(0x1100, 222);

        assert(bus.read(0x10FF) == 111);
        assert(bus.read(0x1100) == 222);
    }

    // Overlapping mappings should be rejected.
    {
        Bus bus;

        RAM ram1(256);
        RAM ram2(256);

        bus.attach(ram1, 0x1000, 0x10FF);

        bool exceptionThrown = false;

        try
        {
            bus.attach(ram2, 0x1080, 0x117F);
        }
        catch (const std::invalid_argument&)
        {
            exceptionThrown = true;
        }

        assert(exceptionThrown);
    }

    // Invalid address ranges should be rejected.
    {
        Bus bus;
        RAM ram(256);

        bool exceptionThrown = false;

        try
        {
            bus.attach(ram, 0x2000, 0x1000);
        }
        catch (const std::invalid_argument&)
        {
            exceptionThrown = true;
        }

        assert(exceptionThrown);
    }

    // Unmapped reads should fail.
    {
        Bus bus;
        RAM ram(256);

        bus.attach(ram, 0x1000, 0x10FF);

        bool exceptionThrown = false;

        try
        {
            bus.read(0x2000);
        }
        catch (const std::out_of_range&)
        {
            exceptionThrown = true;
        }

        assert(exceptionThrown);
    }

    // Unmapped writes should fail.
    {
        Bus bus;
        RAM ram(256);

        bus.attach(ram, 0x1000, 0x10FF);

        bool exceptionThrown = false;

        try
        {
            bus.write(0x2000, 123);
        }
        catch (const std::out_of_range&)
        {
            exceptionThrown = true;
        }

        assert(exceptionThrown);
    }
	
	//prove overflow protection
	{
		Bus bus;
		RAM ram(5);

		bool threw = false;

		try
		{
			bus.attach(
				ram,
				0xFFFFFFFE,
				0x00000002
			);
		}
		catch (const std::invalid_argument&)
		{
			threw = true;
		}

		assert(threw);
	}

    std::cout << "Bus tests passed.\n";

    return 0;
}
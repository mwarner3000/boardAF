#include <cassert>
#include <cstdint>
#include <stdexcept>

#include "Devices/ADC.h"
#include "Devices/ADCConfig.h"
#include "Devices/GPIO.h"
#include "Interrupts/InterruptController.h"

int main()
{
    GPIO gpio(8, 5.0, 2.5);
    InterruptController interruptController;

    ADCConfig config;
	
	//ADC 1
    ADC adc(
        config,
        gpio,
        interruptController
    );
	
	gpio.getPin(0).setExternalVoltage(2.5);
	
	adc.write(0, 0);
	
	adc.write(1, 1);
	
	assert((adc.read(2) & (1u << 0)) != 0);
	
	for (std::uint32_t i = 0;
     i < config.conversionCycles - 1;
     ++i)
	{
		adc.tick(i);
	}
	assert((adc.read(2) & (1u << 0)) != 0); // BUSY
	assert((adc.read(2) & (1u << 1)) == 0); // not COMPLETE
	
	adc.tick(config.conversionCycles - 1);
	
	assert((adc.read(2) & (1u << 0)) == 0);
	
	assert(adc.read(3) == 512);
	
	assert(adc.read(4) == 0);
	
	//ADC 2
	ADC adc2(
		config,
		gpio,
		interruptController
	);
	
	gpio.getPin(0).setExternalVoltage(1.0);

	adc2.write(0, 0);
	adc2.write(1, 1);
	
	gpio.getPin(0).setExternalVoltage(4.0);
	
	for (std::uint32_t i = 0;
		 i < config.conversionCycles;
		 ++i)
	{
		adc2.tick(i);
	}
	
	assert(adc2.read(3) == 205);
	
	//ADC 3
	ADC adc3(
		config,
		gpio,
		interruptController
	);
	
	gpio.getPin(0).setExternalVoltage(1.0);
	gpio.getPin(1).setExternalVoltage(4.0);
	
	adc3.write(0, 0);
	adc3.write(1, 1);
	
	adc3.write(0, 1);
	
	for (std::uint32_t i = 0;
		 i < config.conversionCycles;
		 ++i)
	{
		adc3.tick(i);
	}
	
	assert(adc3.read(3) == 205);
	assert(adc3.read(4) == 0);
	
	adc3.write(1, 1);
	
	for (std::uint32_t i = 0;
		 i < config.conversionCycles;
		 ++i)
	{
		adc3.tick(i);
	}
	
	assert(adc3.read(3) == 818);
	assert(adc3.read(4) == 1);
	
	//ADC 4
	ADC adc4(
		config,
		gpio,
		interruptController
	);

	gpio.getPin(0).setExternalVoltage(1.0);

	adc4.write(0, 0);
	adc4.write(1, 1);
	
	for (std::uint32_t i = 0;
		 i < config.conversionCycles / 2;
		 ++i)
	{
		adc4.tick(i);
	}
	
	adc4.write(1, 1);
	
	for (std::uint32_t i = config.conversionCycles / 2;
		 i < config.conversionCycles;
		 ++i)
	{
		adc4.tick(i);
	}
	
	assert((adc4.read(2) & (1u << 0)) == 0); // not BUSY
	assert((adc4.read(2) & (1u << 1)) != 0); // COMPLETE
	assert(adc4.read(3) == 205);
	
	//ADC 5
	ADC adc5(
		config,
		gpio,
		interruptController
	);
	
	adc5.write(0, 12);
	adc5.write(1, 1);
	
	assert((adc5.read(2) & (1u << 0)) == 0); // not BUSY
	assert((adc5.read(2) & (1u << 3)) != 0); // INVALID_CHANNEL
	
	assert(adc5.read(3) == 0);
	assert(adc5.read(4) == 0);
	
	adc5.write(2, (1u << 3));
	
	assert((adc5.read(2) & (1u << 3)) == 0);
	
	//ADC 6
	ADC adc6(
		config,
		gpio,
		interruptController
	);
	
	gpio.getPin(0).setExternalVoltage(1.0);

	adc6.write(0, 0);
	adc6.write(1, 1);

	for (std::uint32_t i = 0;
		 i < config.conversionCycles;
		 ++i)
	{
		adc6.tick(i);
	}
	
	gpio.getPin(0).setExternalVoltage(4.0);

	adc6.write(1, 1);

	for (std::uint32_t i = 0;
		 i < config.conversionCycles;
		 ++i)
	{
		adc6.tick(i);
	}
	
	assert((adc6.read(2) & (1u << 2)) != 0); // OVERRUN
	assert(adc6.read(3) == 818);              // newest result
	
	adc6.write(2, (1u << 2));

	assert((adc6.read(2) & (1u << 2)) == 0);
	
	//ADC 7
	ADC adc7(
		config,
		gpio,
		interruptController
	);
	
	gpio.getPin(0).setExternalVoltage(1.0);

	adc7.write(0, 0);
	adc7.write(1, 1);

	for (std::uint32_t i = 0;
		 i < config.conversionCycles;
		 ++i)
	{
		adc7.tick(i);
	}
	
	assert(adc7.read(3) == 205);
	
	gpio.getPin(0).setExternalVoltage(4.0);

	adc7.write(1, 1);

	for (std::uint32_t i = 0;
		 i < config.conversionCycles;
		 ++i)
	{
		adc7.tick(i);
	}
	
	assert((adc7.read(2) & (1u << 2)) == 0);
	assert(adc7.read(3) == 818);
	
	//ADC 8
	ADC adc8(
		config,
		gpio,
		interruptController
	);

	gpio.getPin(0).setExternalVoltage(2.5);

	adc8.write(0, 0);
	adc8.write(1, 1);

	for (std::uint32_t i = 0;
		 i < config.conversionCycles;
		 ++i)
	{
		adc8.tick(i);
	}
	
	assert((adc8.read(2) & (1u << 1)) != 0);
	
	assert(adc8.read(3) == 512);
	
	assert((adc8.read(2) & (1u << 1)) != 0);
	
	adc8.write(2, (1u << 1));
	
	assert((adc8.read(2) & (1u << 1)) == 0);
	
	//ADC 9
	ADC adc9(
		config,
		gpio,
		interruptController
	);
	
	//enable interrupts through register 5
	adc9.write(5, 1);
	
	//set up and run a conversion
	gpio.getPin(0).setExternalVoltage(2.5);

	adc9.write(0, 0);
	adc9.write(1, 1);

	for (std::uint32_t i = 0;
		 i < config.conversionCycles;
		 ++i)
	{
		adc9.tick(i);
	}
	
	//verify controller sees pending IRQ
    assert(
		interruptController.getNextPending() == 
		config.interruptNumber);
		
	interruptController.clear(config.interruptNumber);
		
	//ADC 10
	ADC adc10(
		config,
		gpio,
		interruptController
	);

	gpio.getPin(0).setExternalVoltage(2.5);

	adc10.write(0, 0);
	adc10.write(1, 1);

	for (std::uint32_t i = 0;
		 i < config.conversionCycles;
		 ++i)
	{
		adc10.tick(i);
	}
	
	bool threw = false;

	try
	{
		interruptController.getNextPending();
	}
	catch (const std::runtime_error&)
	{
		threw = true;
	}
	assert(threw);
	
	//ADC 11
	ADC adc11(
		config,
		gpio,
		interruptController
	);
	
	//pin 0 = 3v, pin 1 = floating
	gpio.getPin(0).setExternalVoltage(3.0);
	gpio.getPin(1).clearExternalVoltage();
	
	//convert ch 0
	adc11.write(0, 0);
	adc11.write(1, 1);

	for (std::uint32_t i = 0;
		 i < config.conversionCycles;
		 ++i)
	{
		adc11.tick(i);
	}
	
	//read the result
	assert(adc11.read(3) == 614);
	
	//convert ch 1
	adc11.write(0, 1);
	adc11.write(1, 1);

	for (std::uint32_t i = 0;
		 i < config.conversionCycles;
		 ++i)
	{
		adc11.tick(i);
	}
	
	//verifyassert(adc11.read(3) == 614);
	assert(adc11.read(4) == 1);
	
	{
		ADCConfig config;

		// Use the same GPIO constructor/setup
		// that your existing ADC tests use.
		GPIO gpio(8, 5.0, 2.5);

		InterruptController interruptController(8);

		ADC adc(
			config,
			gpio,
			interruptController
		);

		adc.write(5, 1); // INTERRUPT_ENABLE
		adc.write(0, 0); // CHANNEL
		adc.write(1, 1); // START

		for (std::uint32_t i = 0;
			 i < config.conversionCycles;
			 ++i)
		{
			adc.tick(i);
		}

		assert(
			(adc.read(2) & (1u << 1)) != 0
		);

		assert(
			interruptController.isPending(
				config.interruptNumber
			)
		);

		// Simulate CPU acknowledgement.
		interruptController.clear(
			config.interruptNumber
		);

		// COMPLETE is still asserted.
		assert(
			interruptController.isPending(
				config.interruptNumber
			)
		);

		// Firmware clears COMPLETE.
		adc.write(2, (1u << 1));

		assert(
			!interruptController.isPending(
				config.interruptNumber
			)
		);
	}
	
	{
		ADCConfig config;

		GPIO gpio(8, 5.0, 2.5);

		InterruptController interruptController(8);

		ADC adc(
			config,
			gpio,
			interruptController
		);

		adc.write(0, 0); // CHANNEL
		adc.write(1, 1); // START

		for (std::uint32_t i = 0;
			 i < config.conversionCycles;
			 ++i)
		{
			adc.tick(i);
		}

		// Conversion finished, but IRQ is disabled.
		assert(
			(adc.read(2) & (1u << 1)) != 0
		);

		assert(
			!interruptController.isPending(
				config.interruptNumber
			)
		);

		// Enable IRQ while COMPLETE is already set.
		adc.write(5, 1);

		assert(
			interruptController.isPending(
				config.interruptNumber
			)
		);

		// Disable IRQ again.
		adc.write(5, 0);

		assert(
			!interruptController.isPending(
				config.interruptNumber
			)
		);
	}

    return 0;
}
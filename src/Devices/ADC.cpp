#include "Devices/ADC.h"

#include <stdexcept>
#include <cmath>

ADC::ADC(
    const ADCConfig& config,
    GPIO& gpio,
    InterruptController& interruptController
)
    : config(config),
      gpio(gpio),
      interruptController(interruptController)
{
	if (config.channelCount == 0)
	{
		throw std::invalid_argument(
			"ADC channel count must be greater than zero"
		);
	}
	
	if (config.resolutionBits < 1 || config.resolutionBits > 16)
	{
		throw std::invalid_argument(
			"ADC resolution must be between 1 and 16 bits"
		);
	}
	
	if (config.referenceVoltage <= 0.0)
	{
		throw std::invalid_argument(
			"ADC reference voltage must be greater than zero"
		);
	}
	
	if (config.conversionCycles == 0)
	{
		throw std::invalid_argument(
			"ADC conversion cycles must be greater than zero"
		);
	}
	
	if (config.channelToPin.size() != config.channelCount)
	{
		throw std::invalid_argument(
			"ADC channel-to-pin mapping count must match channel count"
		);
	}
	
	for (std::size_t pinIndex : config.channelToPin)
	{
		if (pinIndex >= gpio.getPinCount())
		{
			throw std::invalid_argument(
				"ADC channel maps to an invalid GPIO pin"
			);
		}
	}
}


double ADC::sampleSelectedChannel()
{
    std::size_t pinIndex =
        config.channelToPin[selectedChannel];

    Pin& pin = gpio.getPin(pinIndex);
	
	auto voltage = pin.getEffectiveVoltage(
		gpio.getLogicVoltage()
	);
	
	if (voltage.has_value())
	{
		sampledVoltage = voltage.value();
	}
	
	return sampledVoltage;
}

std::uint32_t ADC::read(std::uint32_t address)
{
    switch (address)
    {
        case 0:
            return static_cast<std::uint32_t>(selectedChannel);

        case 1:
            return 0;
			
		case 2:
		{
			std::uint32_t status = 0;

			if (busy)
				status |= (1u << 0);

			if (complete)
				status |= (1u << 1);

			if (overrun)
				status |= (1u << 2);

			if (invalidChannel)
				status |= (1u << 3);

			return status;
		}

        case 3:
            resultUnread = false;
            return result;

        case 4:
            return static_cast<std::uint32_t>(resultChannel);

        case 5:
            return interruptEnable ? 1 : 0;
			
		default:
			throw std::out_of_range(
				"Invalid ADC register address"
			);
    }
}

void ADC::write(
    std::uint32_t address,
    std::uint32_t value
)
{
    switch (address)
    {
        case 0:
            selectedChannel =
                static_cast<std::size_t>(value);
            return;
			
		case 1:
			if ((value & (1u << 0)) == 0)
				return;

			if (busy)
				return;

			if (selectedChannel >= config.channelCount)
			{
				invalidChannel = true;
				return;
			}

			activeChannel = selectedChannel;
			sampledVoltage = sampleSelectedChannel();
			cyclesRemaining = config.conversionCycles;
			busy = true;

			return;
			
		case 2:
			if (value & (1u << 1))
				complete = false;

			if (value & (1u << 2))
				overrun = false;

			if (value & (1u << 3))
				invalidChannel = false;

			updateInterruptLine();
			return;
			
		case 3:
		case 4:
			return;
	
		case 5:
			interruptEnable = (value != 0);
			updateInterruptLine();
			return;
			
		default:
			throw std::out_of_range(
				"Invalid ADC register address"
			);
	}
	
}

void ADC::tick(std::uint64_t cycle)
{
    if (!busy)
        return;

    if (cyclesRemaining > 0)
        --cyclesRemaining;

    if (cyclesRemaining > 0)
        return;

    if (resultUnread)
		overrun = true;

	result = convertSampleToCode();
	resultChannel = activeChannel;
	resultUnread = true;

	complete = true;
	busy = false;

	updateInterruptLine();
}

std::uint16_t ADC::convertSampleToCode() const
{
    std::uint32_t maxCode =
        (1u << config.resolutionBits) - 1u;

    if (sampledVoltage <= 0.0)
        return 0;

    if (sampledVoltage >= config.referenceVoltage)
        return static_cast<std::uint16_t>(maxCode);
	
	double normalized =
		sampledVoltage / config.referenceVoltage;

	double scaled =
		normalized * static_cast<double>(maxCode);

	return static_cast<std::uint16_t>(
		std::round(scaled)
	);
}

void ADC::updateInterruptLine()
{
    interruptController.setLine(
        config.interruptNumber,
        complete && interruptEnable
    );
}
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

struct ADCConfig
{
	std::size_t channelCount = 8;
	std::uint8_t resolutionBits = 10;
	double referenceVoltage = 5.0;
	std::uint32_t conversionCycles = 20;
	std::size_t interruptNumber = 2;
	std::uint32_t baseAddress = 0x00003000;
	
	std::vector<std::size_t> channelToPin{
		0, 1, 2, 3, 4, 5, 6, 7
	};
};
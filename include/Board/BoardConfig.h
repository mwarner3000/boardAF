#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Devices/ADCConfig.h"
#include "Devices/TimerConfig.h"

struct BoardConfig
{
    // Core hardware
    std::uint64_t clockHz = 16'000'000;
    std::size_t ramWords = 1024;
    std::size_t gpioPins = 8;
	std::size_t interruptCount = 8;
	std::size_t registerCount = 8;

    // Electrical configuration
    double logicVoltage = 5.0;
    double digitalHighThreshold = 2.5;

    // Memory map
    std::uint32_t ramBase   = 0x00000000;
    std::uint32_t gpioBase  = 0x00001000;
	std::vector<TimerConfig> timers{
		TimerConfig{
			0x00002000,
			0
		}
	};
	
	//ADC configuration
		std::vector<ADCConfig> adcs{
		ADCConfig{}
	};
};
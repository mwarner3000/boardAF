#include <cstdint>
#include <stdexcept>

#include "Simulator/Simulator.h"

namespace
{
    std::uint32_t validateRAMSize(const BoardConfig& config)
    {
        if (config.ramWords == 0)
        {
            throw std::invalid_argument(
                "Board RAM size must be greater than zero"
            );
        }

        const std::uint64_t lastAddress =
            static_cast<std::uint64_t>(config.ramBase) +
            static_cast<std::uint64_t>(config.ramWords) - 1;

        if (lastAddress > UINT32_MAX)
        {
            throw std::invalid_argument(
                "Board RAM exceeds the 32-bit address space"
            );
        }

        return static_cast<std::uint32_t>(config.ramWords);
    }
}

Simulator::Simulator()
    : Simulator(BoardConfig{})
{
}

Simulator::Simulator(const BoardConfig& config)
    : config(config),
      ram(validateRAMSize(config)),
      gpio(
			config.gpioPins,
			config.logicVoltage, 
			config.digitalHighThreshold
		),
	  interruptController(config.interruptCount),
	  cpu(
			bus,
			interruptController,
			config.registerCount
		)
{
    bus.attach(
		ram,
		config.ramBase,
		config.ramBase +
			static_cast<std::uint32_t>(config.ramWords - 1)
	);

	bus.attach(
		gpio,
		config.gpioBase,
		config.gpioBase +
			GPIO::RegisterCount - 1
	);
	
	if (config.interruptCount == 0)
	{
		throw std::invalid_argument(
			"Board interrupt count must be greater than zero"
		);
	}
	
	constexpr std::uint64_t MaxClockHz = 1'000'000'000ULL;
	
	if (config.clockHz == 0)
	{
		throw std::invalid_argument(
			"Board clock frequency must be greater than zero"
		);
	}
	
	if (config.clockHz > MaxClockHz)
	{
		throw std::invalid_argument(
			"Board clock frequency cannot exceed 1 GHz"
		);
	}
	
	for (const TimerConfig& timerConfig : config.timers)
	{
		if (timerConfig.interruptNumber >= config.interruptCount)
		{
			throw std::invalid_argument(
				"Timer interrupt number is outside the configured interrupt range"
			);
		}
	
		timers.emplace_back(
			interruptController,
			timerConfig.interruptNumber
		);
	}
	
	for (const ADCConfig& adcConfig : config.adcs)
	{
		if (adcConfig.interruptNumber >= config.interruptCount)
		{
			throw std::invalid_argument(
				"ADC interrupt number is outside the configured interrupt range"
			);
		}
		
		adcs.emplace_back(
			adcConfig,
			gpio,
			interruptController
		);
	}

	for (std::size_t i = 0; i < timers.size(); ++i)
	{
		bus.attach(
			timers[i],
			config.timers[i].baseAddress,
			config.timers[i].baseAddress +
				Timer::RegisterCount - 1
		);
	}
	
	for (std::size_t i = 0; i < adcs.size(); ++i)
	{
		bus.attach(
			adcs[i],
			config.adcs[i].baseAddress,
			config.adcs[i].baseAddress +
				ADC::RegisterCount - 1
		);
	}

    clockables.push_back(&cpu);
	
	for (ADC& adc : adcs)
	{
		clockables.push_back(&adc);
	}
	
	for (Timer& timer : timers)
	{
		clockables.push_back(&timer);
	}
}

Bus& Simulator::getBus()
{
    return bus;
}

RAM& Simulator::getRAM()
{
    return ram;
}

GPIO& Simulator::getGPIO()
{
    return gpio;
}

Clock& Simulator::getClock()
{
    return clock;
}

void Simulator::tick()
{
    clock.tick();

    for (IClockable* device : clockables)
    {
        device->tick(clock.getCycle());
    }
}

void Simulator::addClockable(IClockable& device)
{
    clockables.push_back(&device);
}

Timer& Simulator::getTimer()
{
    return getTimer(0);
}

Timer& Simulator::getTimer(std::size_t index)
{
    return timers.at(index);
}

const Timer& Simulator::getTimer() const
{
    return getTimer(0);
}

const Timer& Simulator::getTimer(std::size_t index) const
{
    return timers.at(index);
}

SimpleCPU& Simulator::getCPU()
{
    return cpu;
}

void Simulator::run(std::uint64_t maxCycles)
{
    const std::uint64_t startCycle = clock.getCycle();

    while (clock.getCycle() - startCycle < maxCycles)
    {
        tick();
    }
}

const BoardConfig& Simulator::getConfig() const
{
    return config;
}

void Simulator::advanceCycles(std::uint64_t cycles)
{
    for (std::uint64_t i = 0; i < cycles; ++i)
    {
        tick();
    }
}

void Simulator::advanceTime(
    std::chrono::nanoseconds duration
)
{
    if (duration.count() < 0)
    {
        throw std::invalid_argument(
            "Simulation time duration cannot be negative"
        );
    }

    constexpr std::uint64_t NanosecondsPerSecond =
        1'000'000'000ULL;

    const std::uint64_t nanoseconds =
        static_cast<std::uint64_t>(
            duration.count()
        );

    const std::uint64_t wholeSeconds =
        nanoseconds / NanosecondsPerSecond;

    const std::uint64_t remainingNanoseconds =
        nanoseconds % NanosecondsPerSecond;

    const std::uint64_t wholeSecondCycles =
        wholeSeconds * config.clockHz;

    const std::uint64_t scaledFraction =
        remainingNanoseconds * config.clockHz +
        timeRemainder;

    const std::uint64_t fractionalCycles =
        scaledFraction / NanosecondsPerSecond;

    timeRemainder =
        scaledFraction % NanosecondsPerSecond;

    const std::uint64_t cycles =
        wholeSecondCycles + fractionalCycles;

    advanceCycles(cycles);
}

void Simulator::startRealTime()
{
    lastRealTimeUpdate =
        std::chrono::steady_clock::now();

    realTimeRunning = true;
}

void Simulator::updateRealTime()
{
    if (!realTimeRunning)
    {
        return;
    }

    const auto now =
        std::chrono::steady_clock::now();

    const auto elapsed =
        std::chrono::duration_cast<
            std::chrono::nanoseconds
        >(
            now - lastRealTimeUpdate
        );

    lastRealTimeUpdate = now;

    advanceTime(elapsed);
}

void Simulator::stopRealTime()
{
    realTimeRunning = false;
}

bool Simulator::isRealTimeRunning() const
{
    return realTimeRunning;
}

void Simulator::setPinVoltage(
    std::size_t pin,
    double voltage
)
{
    gpio.getPin(pin).setExternalVoltage(voltage);
}

std::optional<double> Simulator::getPinVoltage(std::size_t pinIndex) const
{
    return gpio.getPin(pinIndex).getEffectiveVoltage(
        gpio.getLogicVoltage()
    );
}

InterruptController&
Simulator::getInterruptController()
{
    return interruptController;
}

std::chrono::nanoseconds Simulator::getNextCycleDuration()
{
	std::int64_t numerator = 1000000000 + clockTimeRemainder;
	std::int64_t wholeNanoseconds = numerator / config.clockHz;
	clockTimeRemainder = numerator % config.clockHz;
	
	return std::chrono::nanoseconds(wholeNanoseconds);
}	

ADC& Simulator::getADC()
{
    return getADC(0);
}

ADC& Simulator::getADC(std::size_t index)
{
    return adcs.at(index);
}

const ADC& Simulator::getADC() const
{
    return getADC(0);
}

const ADC& Simulator::getADC(std::size_t index) const
{
    return adcs.at(index);
}
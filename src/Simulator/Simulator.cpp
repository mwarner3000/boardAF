#include <stdexcept>

#include "Simulator/Simulator.h"

Simulator::Simulator()
    : Simulator(BoardConfig{})
{
}

Simulator::Simulator(const BoardConfig& config)
    : config(config),
      ram(config.ramWords),
      gpio(
			config.gpioPins,
			config.logicVoltage, 
			config.digitalHighThreshold
		),
	  interruptController(config.interruptCount),
	  canController(interruptController, 1),
	  cpu(
			bus,
			interruptController,
			config.registerCount
		)
{	
	if (config.ramWords == 0)
	{
		throw std::invalid_argument(
			"Board RAM size must be greater than zero"
		);
	}

    bus.attach(
		ram,
		config.ramBase,
		config.ramBase +
			static_cast<std::uint32_t>(config.ramWords - 1)
	);

	bus.attach(
		gpio,
		config.gpioBase,
		config.gpioBase + 3
	);
	
	if (config.interruptCount == 0)
	{
		throw std::invalid_argument(
			"Board interrupt count must be greater than zero"
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
			config.timers[i].baseAddress + 4
		);
	}
	
	bus.attach(
		canController,
		config.canBase,
		config.canBase + 24
	);
	
	for (std::size_t i = 0; i < adcs.size(); ++i)
	{
		bus.attach(
			adcs[i],
			config.adcs[i].baseAddress,
			config.adcs[i].baseAddress + 5
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

CANController& Simulator::getCANController()
{
    return canController;
}

RunResult Simulator::run(std::uint64_t maxCycles)
{
    std::uint64_t startCycle = clock.getCycle();

    while (!cpu.isHalted())
    {
        if (clock.getCycle() - startCycle >= maxCycles)
        {
            return RunResult::CycleLimitReached;
        }

        tick();
    }

    return RunResult::Halted;
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
    if (config.clockHz == 0)
    {
        throw std::invalid_argument(
            "Board clock frequency must be greater than zero"
        );
    }

    constexpr std::uint64_t NanosecondsPerSecond =
        1'000'000'000ULL;

    const std::uint64_t nanoseconds =
        static_cast<std::uint64_t>(
            duration.count()
        );

    const std::uint64_t scaledTime =
        nanoseconds * config.clockHz +
        timeRemainder;

    const std::uint64_t cycles =
        scaledTime / NanosecondsPerSecond;

    timeRemainder =
        scaledTime % NanosecondsPerSecond;

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

double Simulator::getPinVoltage(
    std::size_t pin
) const
{
	auto voltage =
		gpio.getPin(pin).getEffectiveVoltage(
			config.logicVoltage
		);

	if (!voltage.has_value())
	{
		// TODO: define public API behavior for floating pins.
		return 0.0;
	}

	return voltage.value();
}

InterruptController&
Simulator::getInterruptController()
{
    return interruptController;
}

std::chrono::nanoseconds Simulator::getTimeCredit() const
{
	return timeCredit;
}

std::chrono::nanoseconds Simulator::getNextCycleDuration()
{
	if (config.clockHz == 0)
	{
		throw std::runtime_error(
			"Clock frequency cannot be zero"
		);
	}

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
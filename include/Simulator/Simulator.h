#pragma once

#include <cstdint>
#include <vector>
#include <chrono>
#include <cstddef>
#include <optional>

#include "Board/BoardConfig.h"
#include "Bus/Bus.h"
#include "CPU/SimpleCPU.h"
#include "Devices/GPIO.h"
#include "Devices/Timer.h"
#include "Memory/RAM.h"
#include "Simulator/Clock.h"
#include "Simulator/IClockable.h"
#include "Interrupts/InterruptController.h"
#include "Communication/CANController.h"
#include "Devices/ADC.h"

enum class RunResult
{
    Halted,
    CycleLimitReached
};

class Simulator
{
public:
    Simulator();
    explicit Simulator(const BoardConfig& config);

    Bus& getBus();
    RAM& getRAM();
    GPIO& getGPIO();
    Timer& getTimer();
	const Timer& getTimer() const;
	Timer& getTimer(std::size_t index);
	const Timer& getTimer(std::size_t index) const;
    SimpleCPU& getCPU();
    Clock& getClock();
	InterruptController& getInterruptController();

    const BoardConfig& getConfig() const;

    void tick();
    RunResult run(std::uint64_t maxCycles);

    void addClockable(IClockable& device);
	
	void advanceCycles(std::uint64_t cycles);

	void advanceTime(
		std::chrono::nanoseconds duration
	);
	
	void startRealTime();
	void updateRealTime();
	void stopRealTime();

	bool isRealTimeRunning() const;
	
	void setPinVoltage(std::size_t pin, double voltage);
	std::optional<double> getPinVoltage(std::size_t pinIndex) const;
	std::chrono::nanoseconds getTimeCredit() const;
	std::chrono::nanoseconds getNextCycleDuration();
	
	ADC& getADC();
	const ADC& getADC() const;
	ADC& getADC(std::size_t index);
	const ADC& getADC(std::size_t index) const;

private:
    BoardConfig config;

    Bus bus;
    RAM ram;
    GPIO gpio;
	InterruptController interruptController;
	std::vector<ADC> adcs;

	std::vector<Timer> timers;
	SimpleCPU cpu;

    Clock clock;

    std::vector<IClockable*> clockables;
    std::uint64_t timeRemainder = 0;

    bool realTimeRunning = false;

    std::chrono::steady_clock::time_point
        lastRealTimeUpdate;
	std::chrono::nanoseconds timeCredit{0};
	std::uint64_t clockTimeRemainder{0};
};
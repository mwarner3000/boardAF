#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Bus/IBusDevice.h"
#include "Devices/Pin.h"

class GPIO : public IBusDevice
{
public:
    GPIO(
        std::size_t pinCount = 8,
        double logicVoltage = 5.0,
        double digitalHighThreshold = 2.5
    );

    std::uint32_t read(std::uint32_t address) override;

    void write(
        std::uint32_t address,
        std::uint32_t value
    ) override;

    Pin& getPin(std::size_t index);
    const Pin& getPin(std::size_t index) const;

    std::size_t getPinCount() const;
	
	double getLogicVoltage() const;
	static constexpr std::uint32_t RegisterCount = 4;

private:
    std::size_t selectedPin;

    double logicVoltage;
    double digitalHighThreshold;

    std::vector<Pin> pins;
};
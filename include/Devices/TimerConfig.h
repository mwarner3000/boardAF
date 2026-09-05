#pragma once

#include <cstddef>
#include <cstdint>

struct TimerConfig
{
    std::uint32_t baseAddress = 0;
    std::size_t interruptNumber = 0;
};
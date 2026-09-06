#include "Simulation/Simulation.h"

#include <stdexcept>

Simulation::Simulation ()
	: elapsedTime(0)
{
}

Simulator& Simulation::createNode()
{
    nodes.push_back(
        std::make_unique<Simulator>()
    );

    Simulator& node = *nodes.back();

    return node;
}

Simulator& Simulation::createNode(
    const BoardConfig& config
)
{
    nodes.push_back(
        std::make_unique<Simulator>(config)
    );

    Simulator& node = *nodes.back();

    return node;
}

Simulator& Simulation::getNode(std::size_t index)
{
    if (index >= nodes.size())
    {
        throw std::out_of_range(
            "Invalid simulation node index"
        );
    }

    return *nodes[index];
}

const Simulator& Simulation::getNode(std::size_t index) const
{
    if (index >= nodes.size())
    {
        throw std::out_of_range(
            "Invalid simulation node index"
        );
    }

    return *nodes[index];
}

std::size_t Simulation::getNodeCount() const
{
    return nodes.size();
}

std::chrono::nanoseconds Simulation::getCurrentTime() const
{
	return elapsedTime;
}

void Simulation::advanceTime(
    std::chrono::nanoseconds duration
)
{
    elapsedTime += duration;

    for (auto& node : nodes)
    {
        node->advanceTime(duration);
    }
}

#include <glm/fwd.hpp>
#include <chrono>
#include <cstdint>
#include <thread>
#include <iostream>
#include "myen.hpp"

using my_clock = std::chrono::steady_clock;
auto next_frame = my_clock::now();


void timeSync()
{
    next_frame += std::chrono::milliseconds(1000 / 60); // 5Hz
    std::this_thread::sleep_until(next_frame);
}

int main()
{
    myen::Myen myen{};
    std::cout << "Bye bye 👋" << std::endl;
}

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
    auto model = myen.importMesh("/home/orvergon/myen/assets/obj/monke/monke.glb");
    auto entityId = myen.createEntity(model, glm::vec3(1.0f));
    auto entity = myen.getEntity(entityId);
    while(myen.nextFrame()){
	continue;
    }
    std::cout << "Bye bye 👋" << std::endl;
}

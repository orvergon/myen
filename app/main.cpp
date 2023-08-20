#include <glm/fwd.hpp>
#include <chrono>
#include <cstdint>
#include <thread>
#include <iostream>
#include "imgui.h"
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
    auto frame = 0;
    myen.addUICommands("teste", [&] {
	ImGui::Text("Hi?");
	ImGui::Text("Do closures work here? (%d)", frame);
    });
    while(myen.nextFrame()){
	if(myen.keyPressed("8")){
	    std::cout << "apertou 8" << std::endl;
	    entity->pos += glm::vec3(1.0f, 0.0f, 0.0f);
	}
	frame++;
    }
    std::cout << "Bye bye 👋" << std::endl;
}

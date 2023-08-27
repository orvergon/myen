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
    auto model = myen.importGlftFile("/home/orvergon/myen/assets/obj/monke/monke.glb");
    auto entityId = myen.createEntity(model, glm::vec3(1.0f));
    auto entity = myen.getEntity(entityId);

    while(myen.nextFrame())
    {
	//Camera movement
	glm::vec3 cameraMovement = glm::vec3(0.0f);
	if(myen.keyPressed("w"))
	{
	    cameraMovement += glm::vec3(0.0f, 0.0f, -1.0f);
	}
	if(myen.keyPressed("s"))
	{
	    cameraMovement += glm::vec3(0.0f, 0.0f, 1.0f);
	}
	if(myen.keyPressed("a"))
	{
	    cameraMovement += glm::vec3(1.0f, 0.0f, 0.0f);
	}
	if(myen.keyPressed("d"))
	{
	    cameraMovement += glm::vec3(-1.0f, 0.0f, 0.0f);
	}
	if(cameraMovement != glm::vec3(0.0f))
	{
	    cameraMovement = glm::normalize(cameraMovement);
	    cameraMovement *= 0.2f;
	    myen.camera->cameraPos.x += cameraMovement.x;
	    myen.camera->cameraPos.y += cameraMovement.y;
	    myen.camera->cameraPos.z += cameraMovement.z;
	}
    }

    std::cout << "Bye bye 👋" << std::endl;
}

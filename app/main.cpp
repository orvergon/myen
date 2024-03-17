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

void cameraMovement(myen::Myen &_myen) {
    //Keyboard movement
    glm::vec3 cameraMovement = glm::vec3(0.0f);
    if(_myen.keyPressed(";"))
    {
	_myen.toggleMouseCursor();
    }
    if(_myen.keyPressed("w"))
    {
	cameraMovement += glm::vec3(0.0f, 0.0f, -1.0f);
    }
    if(_myen.keyPressed("s"))
    {
	cameraMovement += glm::vec3(0.0f, 0.0f, 1.0f);
    }
    if(_myen.keyPressed("a"))
    {
	cameraMovement += glm::vec3(1.0f, 0.0f, 0.0f);
    }
    if(_myen.keyPressed("d"))
    {
	cameraMovement += glm::vec3(-1.0f, 0.0f, 0.0f);
    }
    if(cameraMovement != glm::vec3(0.0f))
    {
	cameraMovement = glm::normalize(cameraMovement);
	cameraMovement *= 0.05f;
	_myen.camera->cameraPos.x += cameraMovement.x;
	_myen.camera->cameraPos.y += cameraMovement.y;
	_myen.camera->cameraPos.z += cameraMovement.z;
    }

    //Mouse movement
    float sensitivity = 0.1;
    auto mouseMovement = _myen.getMouseMovement();
    if(mouseMovement.x != 0 or mouseMovement.y != 0)
    {
	_myen.camera->Yaw -= mouseMovement.x*sensitivity;
	_myen.camera->Pitch -= mouseMovement.y*sensitivity;
    }
}

int main()
{
    myen::Myen _myen{myen::MyenConfig{}};
    auto model = _myen.importGlftFile("/home/orvergon/myen/assets/obj/monke/monke.glb");
    auto entityId = _myen.createEntity(model, glm::vec3(1.0f));
    auto entity = _myen.getEntity(entityId);
    auto light = _myen.createLight(glm::vec3(2.0, 0.0, 3.0));
    auto light2 = _myen.createLight(glm::vec3(0.0, 3.0, 0.0));

    while(_myen.nextFrame())
    {
	cameraMovement(_myen);
    }

    std::cout << "Bye bye 👋" << std::endl;
}

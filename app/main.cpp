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
    glm::vec3 cameraMovement = glm::vec3(0.0f);
    glm::vec3 camera2DFront = glm::vec3(_myen.camera->cameraDirection.x, 0.0f, _myen.camera->cameraDirection.z);
    glm::vec3 camera2DRight = glm::vec3(_myen.camera->cameraRight.x, 0.0f, _myen.camera->cameraRight.z);
    static auto time = my_clock::now();
    if(_myen.keyPressed(";"))
    {
	auto new_time = my_clock::now();
	auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(new_time - time).count();
	if(ms >= 1000)
	{
	    _myen.toggleMouseCursor();
	    time = new_time;
	}
    }
    if(_myen.keyPressed("w"))
    {
	cameraMovement += camera2DFront;
    }
    if(_myen.keyPressed("s"))
    {
	cameraMovement -= camera2DFront;
    }
    if(_myen.keyPressed("a"))
    {
	cameraMovement -= camera2DRight;
    }
    if(_myen.keyPressed("d"))
    {
	cameraMovement += camera2DRight;
    }
    if(_myen.keyPressed("q"))
    {
	cameraMovement -= glm::vec3(0.0f, 1.0f, 0.0f);
    }
    if(_myen.keyPressed("e"))
    {
	cameraMovement += glm::vec3(0.0f, 1.0f, 0.0f);
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
    auto entityId = _myen.createEntity(model, glm::vec3(2.0f));
    auto entity2Id = _myen.createEntity(model, glm::vec3(0.0f));
    auto entity1 = _myen.getEntity(entityId);
    auto entity2 = _myen.getEntity(entity2Id);
    std::cout << entity1->pos.x << "|" << entity2->pos.y << std::endl;
    auto light = _myen.createLight(glm::vec3(2.0, 0.0, 3.0));
    auto light2 = _myen.createLight(glm::vec3(0.0, 3.0, 0.0));

    while(_myen.nextFrame())
    {
	cameraMovement(_myen);
    }

    std::cout << "Bye bye 👋" << std::endl;
}

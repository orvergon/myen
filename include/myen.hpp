#pragma once

#include "common.hpp"
#include "renderBackend.hpp"
#include "window.hpp"
#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>
#include <string>
#include <unordered_map>
namespace myen {

typedef uint32_t ModelId;
typedef uint32_t EntityId;

struct Model {
    ModelId id;
    RenderBackend::MeshId meshId;
    common::Mesh mesh;
    common::Texture texture;
};

// Entity e o "model" do render backend deveriam ser bem atrelados
struct Entity {
    EntityId id;
    glm::vec3 pos;
    RenderBackend::ModelId modelId; //Isso não deveria ser publico
    //precisa ter um modelId? porque eu não crio um ID de entidade e uso ele
    //como id do model?
};

struct Camera : common::Camera{
    float farPlane = 100.0f;
    float nearPlane = 0.1f;
    float FOV = glm::radians(45.0f);
    float aspectRatio = 16.0/9.0;
    glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 worldUp = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 cameraRight;
    void updateCamera();
};

class Myen
{
public:
    Myen();
    ~Myen();

    bool nextFrame();
    ModelId importMesh(std::string gltf_path);
    EntityId createEntity(ModelId model, glm::vec3 pos = glm::vec3(0.0f));
    Entity* getEntity(EntityId id);
    bool keyPressed(std::string key);
    void addUICommands(std::string windowName, std::function<void(void)> function);

    Camera* camera;

private:
    Window* window;
    RenderBackend::RenderBackend* renderBackend;
    std::unordered_map<ModelId, Model> models; 
    std::unordered_map<EntityId, Entity> entities; 
    std::unordered_map<std::string, bool> keyPressedMap;
};

};

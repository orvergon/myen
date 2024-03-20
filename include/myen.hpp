#pragma once

#include "common.hpp"
#include "renderBackend.hpp"
#include "window.hpp"
#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>
#include <optional>
#include <string>
#include <unordered_map>
namespace myen {

typedef uint32_t ModelId;
typedef uint32_t EntityId;

/*
  This is an internal representation of a graphical asset.
  Here is where all "graphical" things should be (eg. mesh, texture, etc)
  This is what is registered in the renderer as a 
 */
struct Model {
    ModelId id;
    RenderBackend::MeshId meshId;
    common::Mesh mesh;
    common::Texture texture;
    RenderBackend::ImageId textureId;
};

// Entity e o "model" do render backend deveriam ser bem atrelados
struct Entity {
    enum Type{Graphical, Light};

    EntityId id;
    Type type;
    glm::vec3 pos;
    glm::vec3 rotation;
    std::optional<RenderBackend::ModelId> modelId; //Isso não deveria ser publico
    //precisa ter um modelId? porque eu não crio um ID de entidade e uso ele
    //como id do model no render?
};

//Why does common::Camera exist?
struct Camera : common::Camera{
    float farPlane = 100.0f;
    float nearPlane = 0.1f;
    float FOV = glm::radians(45.0f);
    float aspectRatio = 16.0/9.0;
    glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 worldUp = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 cameraRight;
    float Yaw = 0;
    float Pitch = 0;
    void updateCamera();
};

struct MyenConfig {
    int witdh = 1920;
    int height = 1080;
};

class Myen
{
public:
    Myen(MyenConfig config);
    ~Myen();

    bool nextFrame();
    ModelId importGlftFile(std::string gltf_path);
    EntityId createEntity(ModelId model, glm::vec3 pos = glm::vec3(0.0f));
    EntityId createLight(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 color = glm::vec3(1.0f));
    Entity* getEntity(EntityId id);
    bool keyPressed(std::string key);
    void addUICommands(std::string windowName, std::function<void(void)> function);
    glm::vec2 getMousePos();
    glm::vec2 getMouseMovement();
    void toggleMouseCursor();

    Camera* camera;

private:
    Window* window;
    RenderBackend::RenderBackend* renderBackend;
    std::unordered_map<ModelId, Model> models; 
    std::unordered_map<EntityId, Entity> entities; 
    std::unordered_map<std::string, bool> keyPressedMap;
};

};

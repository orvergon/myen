#pragma once

#include "common.hpp"
#include "renderBackend.hpp"
#include "window.hpp"
#include <cstdint>
#include <glm/fwd.hpp>
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

struct Entity {
    EntityId id;
    glm::vec3 pos;
    RenderBackend::ModelId modelId;
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

private:
    Window* window;
    RenderBackend::RenderBackend* renderBackend;
    std::unordered_map<ModelId, Model> models; 
    std::unordered_map<EntityId, Entity> entities; 
};

};

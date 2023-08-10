#include "myen.hpp"

#include "common.hpp"
#include "stb_image.h"
#include "window.hpp"
#include "renderBackend.hpp"
#include <cstdint>
#include <glm/fwd.hpp>
#include <immintrin.h>
#include <iostream>
#include <chrono>
#include <cstdint>
#include <iterator>
#include <sys/types.h>

#include <string>
#include <sys/types.h>
#include <thread>
#include <vector>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

namespace myen {

std::vector<glm::vec3> get_attribute_vec3(std::string name, tinygltf::Primitive& primitive, tinygltf::Model &model) {
    tinygltf::Accessor& accessor = model.accessors[primitive.attributes[name]];
    tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    const float* attribute = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);

    std::vector<glm::vec3> attributes;
    for (size_t i = 0; i < accessor.count; ++i) {
        attributes.push_back(glm::vec3(attribute[i * 3 + 0],
                                       attribute[i * 3 + 1],
                                       attribute[i * 3 + 2]));
    }
    return attributes;
}

std::vector<glm::vec2> get_attribute_vec2(std::string name, tinygltf::Primitive& primitive, tinygltf::Model &model) {
    tinygltf::Accessor& accessor = model.accessors[primitive.attributes[name]];
    tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    const float* attribute = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);

    std::vector<glm::vec2> attributes;
    for (size_t i = 0; i < accessor.count; ++i) {
        attributes.push_back(glm::vec2(attribute[i * 2 + 0],
                                       attribute[i * 2 + 1]));
    }
    return attributes;
}

std::vector<ushort> get_attribute_ushort(int accessorId, tinygltf::Primitive& primitive, tinygltf::Model &model) {
    tinygltf::Accessor& accessor = model.accessors[accessorId];
    tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
    tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
    const ushort* attribute = reinterpret_cast<const ushort*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);

    std::vector<ushort> attributes;
    for (size_t i = 0; i < accessor.count; ++i) {
        attributes.push_back(attribute[i]);
    }
    return attributes;
}


Myen::Myen()
{
    window = new Window();
    renderBackend = new RenderBackend::RenderBackend(window);
}

Myen::~Myen()
{}

bool Myen::nextFrame() {
    if(window->shouldClose()) {
        return false;
    }

    for(auto& [entityId, entity]: entities){
        renderBackend->updateModelPosition(entity.modelId, entity.pos);
    }
    
    renderBackend->drawFrame();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    return true;
}

ModelId Myen::importMesh(std::string gltf_path) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    common::Mesh m;
    common::Texture t;
    //bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, "/home/orvergon/myen/assets/obj/plane/plane.glb");
    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, "/home/orvergon/myen/assets/obj/monke/monke.glb");
    for (auto& mesh : model.meshes) {
        auto& primitive = mesh.primitives[0]; //There can me more primitives

        auto positions = get_attribute_vec3("POSITION", primitive, model);
        auto texture_coordinate = get_attribute_vec2("TEXCOORD_0", primitive, model);

        m.vertices = std::vector<common::Vertex>();
        for(int i = 0; i < positions.size(); i++){
            m.vertices.push_back(common::Vertex{
                    .pos = positions[i],
                    .texCoord = texture_coordinate[i]
                });
        }

        auto indices = get_attribute_ushort(primitive.indices, primitive, model);
        m.indices = std::vector<uint32_t>(std::begin(indices), std::end(indices));

        auto& material = model.materials[primitive.material];
        auto& texture_info = material.pbrMetallicRoughness.baseColorTexture;
        auto& texture = model.textures[texture_info.index];
        auto& image = model.images[texture.source];
        auto& imageBufferView = model.bufferViews[image.bufferView];
        auto& imageBuffer = model.buffers[imageBufferView.buffer];
        const unsigned char* image_data = imageBuffer.data.data() + imageBufferView.byteOffset;
        int x, y, channels;
	//All the image problems where that my vk::image was aways 4 channels but I was loading 3 channels
	//also the pointer to the number of channels always returns the real number of channels on the file
	//not the returned number
        auto pixels = stbi_load_from_memory(image_data, imageBufferView.byteLength, &x, &y, &channels, 4);
        std::cout << "(x: " << x << ", y: " << y << ")" << std::endl;
        std::cout << "channels: " << channels << ")" << std::endl;
	t = common::Texture{
	    .data = pixels,
	    .data_size = 4 * x * y,
	    .height = y,
	    .width = x,
	    .channels = 4,
	};
    }

    if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty()) {
        printf("Err: %s\n", err.c_str());
    }

    if (!ret) {
        printf("Failed to parse glTF\n");
    }

    auto meshId = renderBackend->addMesh(&m);
    //auto modelId = renderBackend->addModel(meshId, glm::vec3(1.0f), glm::vec3(0.0f), &t);
    static ModelId id = 0;
    models[id] = Model{
	.id = id,
	.meshId = meshId,
	.mesh = m,
	.texture = t
    };
    return id++;
}


EntityId Myen::createEntity(ModelId modelId, glm::vec3 pos) {
    auto model = models[modelId];
    auto entityId = renderBackend->addModel(model.meshId, pos, glm::vec3(0.0f), &model.texture);
    static EntityId id = 0;
    entities[id] = Entity{
	.id = id,
	.modelId = entityId,
    };
    return id++;
}


Entity* Myen::getEntity(EntityId id) {
    return &entities[id];
}

};



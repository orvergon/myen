#include "myen.hpp"

#include "GLFW/glfw3.h"
#include "common.hpp"
#include "stb_image.h"
#include "window.hpp"
#include "renderBackend.hpp"
#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
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
#include <thread>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan_enums.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"
#include "imgui.h"

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


bool skip_cursor_pos = false;
glm::vec2 cursor_pos;
glm::vec2 old_cursor_pos;
glm::vec2 cursor_movement = glm::vec2(0.0f);


void cursor_enter_callback(GLFWwindow* window, int entered)
{
    if (entered)
    {

    }
    else
    {

    }
}


void Camera::updateCamera() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    cameraDirection = glm::normalize(front);
    // also re-calculate the Right and Up vector
    cameraRight= glm::normalize(glm::cross(cameraDirection, worldUp));  
    cameraUp = glm::normalize(glm::cross(cameraRight, cameraDirection));
    proj = glm::perspective(FOV, aspectRatio, nearPlane, farPlane);
    //view = glm::translate(glm::mat4(1.0f), -glm::vec3(cameraPos.x, cameraPos.y, cameraPos.z));
    view = glm::lookAt(glm::vec3(cameraPos), glm::vec3(cameraPos) + cameraDirection, cameraUp);
}



Myen::Myen(MyenConfig config)
{
    window = new Window(config.witdh, config.height);
    glfwSetCursorEnterCallback(window->window, cursor_enter_callback);
    cursor_pos = window->getMousePosition();
    old_cursor_pos = cursor_pos;
    auto surface_size = window->getSurfaceSize();
    camera = new Camera();
    camera->aspectRatio = (float)surface_size.width / (float)surface_size.height;
    framerate = config.framerate;

    renderBackend = new RenderBackend::RenderBackend(window, camera);

    renderBackend->addUICommands("Mouse Position",
    [&]{
	ImGui::Text("(%f, %f)", cursor_movement.x, cursor_movement.y);
	ImGui::Text("(%f, %f)", cursor_pos.x, cursor_pos.y);
    });

    renderBackend->addUICommands("timer info",
    [&]{
	ImGui::Text("Frame time: %ld ms", frameTime.count());
	ImGui::Text("Delay time: %ld ms", (1000/framerate) - frameTime.count());
    });
}


Myen::~Myen()
{}


void Myen::addUICommands(std::string windowName,
                         std::function<void(void)> function)
{
    renderBackend->addUICommands(windowName, function);
}


glm::vec2 Myen::getMousePos()
{
    return cursor_pos;
}


glm::vec2 Myen::getMouseMovement()
{
    return cursor_movement;
}


void Myen::toggleMouseCursor()
{
    window->toggleMouse();
}

bool Myen::nextFrame() {
    if(window->shouldClose()) {
        return false;
    }

    old_cursor_pos = cursor_pos;
    cursor_pos = window->getMousePosition();
    cursor_movement = cursor_pos - old_cursor_pos;

    auto keyEvents = window->getKeyEvents();
    for(auto& keyEvent : keyEvents){
	if(keyEvent.keyStatus == KeyStatus::ePRESSED)
	{
	    keyPressedMap[keyEvent.keyName] = true;
	    //std::cout << "Apertou: " << keyEvent.keyName << std::endl;
	}
	else
	{
	    keyPressedMap[keyEvent.keyName] = false;
	    //std::cout << "Soltou: " << keyEvent.keyName << std::endl;
	}
    }

    for(auto& [entityId, entity]: entities){
	if(entity.type == Entity::Type::Graphical)
	    renderBackend->updateModelPosition(entity.modelId.value(), entity.pos, entity.rotation);
    }
    
    camera->updateCamera();
    renderBackend->drawFrame();
    endFrameTimePoint = std::chrono::steady_clock::now();
    frameTime = std::chrono::duration_cast<std::chrono::milliseconds>(endFrameTimePoint - beginFrameTimePoint);
    auto next_frame = std::chrono::milliseconds((1000/framerate) - frameTime.count());
    std::this_thread::sleep_for(next_frame);
    beginFrameTimePoint = std::chrono::steady_clock::now();
    return true;
}


ModelId Myen::importGlftFile(std::string gltf_path) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    common::Mesh m;
    common::Texture t;
    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, gltf_path);
    for (auto& mesh : model.meshes) {
        auto& primitive = mesh.primitives[0]; //There can me more primitives

        auto positions = get_attribute_vec3("POSITION", primitive, model);
        auto normals = get_attribute_vec3("NORMAL", primitive, model);
        auto texture_coordinate = get_attribute_vec2("TEXCOORD_0", primitive, model);

        m.vertices = std::vector<common::Vertex>();
        for(int i = 0; i < positions.size(); i++){
            m.vertices.push_back(common::Vertex{
                    .pos = positions[i],
		    .normal = normals[i],
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
        //std::cout << "(x: " << x << ", y: " << y << ")" << std::endl;
        //std::cout << "channels: " << channels << ")" << std::endl;
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
    auto textureId = renderBackend->addTexture(&t);
    //auto modelId = renderBackend->addModel(meshId, glm::vec3(1.0f), glm::vec3(0.0f), &t);
    static ModelId id = 0;
    models[id] = Model{
	.id = id,
	.meshId = meshId,
	.mesh = m,
	.texture = t,
	.textureId = textureId,
    };
    return id++;
}

EntityId nextEntityId = 0;

EntityId Myen::createEntity(ModelId modelId, glm::vec3 pos, common::PipelineCreateInfo shaderInfo) {
    auto model = models[modelId];
    auto pipelineId = renderBackend->createPipeline(shaderInfo);
    auto entityId = renderBackend->addModel(model.meshId, pos, glm::vec3(0.0f), model.textureId, pipelineId);
    entities[nextEntityId] = Entity{
	.id = nextEntityId,
	.type = Entity::Type::Graphical,
        .pos = pos,
	.modelId = entityId,
    };
    return nextEntityId++;
}

EntityId Myen::createLight(glm::vec3 pos, glm::vec3 color)
{
    entities[nextEntityId] = Entity{
	.id = nextEntityId,
	.type = Entity::Type::Light,
    };
    renderBackend->addLight(pos, color);
    return nextEntityId++;
}

Entity* Myen::getEntity(EntityId id) {
    return &entities[id];
}


bool Myen::keyPressed(std::string key)
{
    //Trash, I'm searching the key twice
    if(keyPressedMap.find(key) == keyPressedMap.end())
    {
	keyPressedMap[key] = false;
    }

    return keyPressedMap[key];
}

};




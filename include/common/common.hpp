#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <vulkan/vulkan.hpp>

namespace common{
struct Vertex{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct Mesh{
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
};

struct Model {
    
};

struct Texture{
    unsigned char* data;
    int data_size;
    int height;
    int width;
    int channels;
};

struct Camera{
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 cameraPos = glm::vec4(0.0f);
};

class Window{
public:
    virtual std::vector<const char*> getRequiredVulkanExtensions() = 0;
    virtual vk::SurfaceKHR createSurface(vk::Instance instance) = 0;
    virtual vk::Extent2D getSurfaceSize() = 0;
    virtual void toggleMouse() = 0;

    void* windowPointer;
};

// xxx: these enums are hacky but this isn't production code and I don't wanna waste
// processing on a mapping
enum FrontFace {
    Clockwise = (int) vk::FrontFace::eClockwise,
    CounterClockwise = (int) vk::FrontFace::eClockwise,
};

enum CullMode{
    Back = (int) vk::CullModeFlagBits::eBack,
    Front = (int) vk::CullModeFlagBits::eFront,
    FrontAndBack = (int) vk::CullModeFlagBits::eFrontAndBack,
    None = (int) vk::CullModeFlagBits::eNone,
};

struct PipelineCreateInfo {
    common::FrontFace frontFace = FrontFace::Clockwise;
    common::CullMode cullMode = CullMode::Back;
    std::string vertexShaderPath = "/home/orvergon/myen/assets/default-shaders/vert";
    std::string fragmentShaderPath = "/home/orvergon/myen/assets/default-shaders/frag";
};

}


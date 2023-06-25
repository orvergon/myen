#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <vulkan/vulkan.hpp>

namespace common{
struct Vertex{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 texCoord;
};

struct Mesh{
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
};

struct Node{
    std::string nome;
    std::vector<Mesh> mesh;
};

struct Texture{
    unsigned char* data;
    int height;
    int width;
    int channels;
};

struct Camera{
    glm::mat4 view;
    glm::mat4 proj;
    glm::vec4 cameraPos;
};

class Window{
public:
    virtual std::vector<const char*> getRequiredVulkanExtensions() = 0;
    virtual vk::SurfaceKHR createSurface(vk::Instance instance) = 0;
    virtual vk::Extent2D getSurfaceSize() = 0;
};

}


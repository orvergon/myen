#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <vulkan/vulkan.hpp>

namespace common{
struct Vertex{
    glm::vec3 pos;
    //glm::vec3 normal;
    glm::vec2 texCoord;
};

struct Mesh{
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;
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

    void* windowPointer;
};

}


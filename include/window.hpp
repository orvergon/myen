#pragma once

#include <unordered_map>
#include <vector>
#include <array>
#include <vulkan/vulkan.hpp>

#include "common/common.hpp"

//Foward declarations
class GLFWwindow;


struct Window : common::Window
{
public:
    GLFWwindow* window;
    bool* keys;

    Window();
    ~Window();

    bool shouldClose();
    //void createSurface(VkInstance instance, VkSurfaceKHR* surface);
    vk::SurfaceKHR createSurface(vk::Instance instance);
    std::vector<const char*> getRequiredVulkanExtensions();
    vk::Extent2D getSurfaceSize();
    std::array<float , 2> getMouseOffset();

private:
    float _xpos = 0;
    float _ypos = 0;
    std::array<float, 2> mouseOffset;

    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int actions, int mods);
};

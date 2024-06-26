#pragma once

#include <glm/fwd.hpp>
#include <unordered_map>
#include <vector>
#include <array>
#include <vulkan/vulkan.hpp>

#include "common/common.hpp"

//Foward declarations
class GLFWwindow;

enum KeyStatus {
    ePRESSED,
    eRELEASED,
};

struct KeyEvent {
    std::string keyName;
    KeyStatus keyStatus;
};

enum MouseStatus {
    eActive,
    eInactive,
};

struct Window : common::Window
{
public:
    GLFWwindow* window;
    bool* keys;
    MouseStatus mouseStatus = MouseStatus::eActive;

    Window(int width=1600, int height=900);
    ~Window();

    bool shouldClose();
    vk::SurfaceKHR createSurface(vk::Instance instance);
    std::vector<const char*> getRequiredVulkanExtensions();
    vk::Extent2D getSurfaceSize();
    void toggleMouse();
    std::array<float, 2> getMouseOffset();
    glm::vec2 getMousePosition();
    std::vector<KeyEvent> getKeyEvents();

private:
    float _xpos = 0;
    float _ypos = 0;
    std::array<float, 2> mouseOffset;

    static void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int actions, int mods);
};

#include "window.hpp"
#include "common.hpp"
#include <array>
#include <cstdint>
#include <ostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#include <iostream>

Window::Window()
{
    std::cout << "[📃] Window: Constructor." << std::endl;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    window = glfwCreateWindow(1280, 720, "Vulkan window", nullptr, nullptr);
    windowPointer = (void*) window;

    if (glfwRawMouseMotionSupported())
    {
	glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    glfwSetWindowUserPointer(window, this);
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, Window::mouseCallback);
    glfwSetKeyCallback(window, Window::keyCallback);
}

Window::~Window()
{
    std::cout << "[📃] Window: Destructor." << std::endl;
    glfwDestroyWindow(window);
    glfwTerminate();
}

bool Window::shouldClose()
{
    glfwPollEvents();
    return glfwWindowShouldClose(window);
}

std::vector<const char*> Window::getRequiredVulkanExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    for(int i = 0; i < extensions.size(); i++)
    {
	std::cout << extensions[i] << std::endl;
    }
    return extensions;
}

vk::SurfaceKHR Window::createSurface(vk::Instance instance)
{
    VkSurfaceKHR surface;
    auto result = glfwCreateWindowSurface(VkInstance(instance), this->window, nullptr, &surface); 
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    return vk::SurfaceKHR(surface);
}

vk::Extent2D Window::getSurfaceSize()
{
    int height, width;
    glfwGetWindowSize(window, &width, &height);
    return vk::Extent2D(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

std::array<float, 2> Window::getMouseOffset()
{
    auto aux = mouseOffset;
    mouseOffset = std::array<float, 2>{0};
    return aux;
}

static bool useCursor = true;

void Window::mouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if(!useCursor)
	return;

    auto _xpos = static_cast<float>(xpos);
    auto _ypos = static_cast<float>(ypos);

    Window* myWindow = (Window*) glfwGetWindowUserPointer(window);
    myWindow->mouseOffset = std::array<float, 2>{_xpos - myWindow->_xpos, _ypos - myWindow->_ypos};
    myWindow->_xpos = _xpos;
    myWindow->_ypos = _ypos;
}

void Window::keyCallback(GLFWwindow* window, int key, int scancode, int actions, int mods)
{
    Window* myWindow = (Window*) glfwGetWindowUserPointer(window);
    if(key == GLFW_KEY_F1 && actions == GLFW_PRESS)
    {
	useCursor = !useCursor;
	//glfwSetCursorPosCallback(window, setCallback ? Window::mouseCallback : NULL);
	glfwSetInputMode(window, GLFW_CURSOR, useCursor ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }

    bool value = actions == GLFW_RELEASE ? false : true;

    //myWindow->keys[key] = value;
}

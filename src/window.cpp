#include "window.hpp"
#include "common.hpp"
#include <array>
#include <cstdint>
#include <glm/fwd.hpp>
#include <ostream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


#include <iostream>

std::vector<KeyEvent> keyEvents;

Window::Window(int width, int height)
{
    std::cout << "[📃] Window: Constructor." << std::endl;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    window = glfwCreateWindow(width, height, "Vulkan window", nullptr, nullptr);
    windowPointer = (void*) window;

    if (glfwRawMouseMotionSupported())
    {
	glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetWindowUserPointer(window, this);
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

void Window::toggleMouse()
{
    switch (mouseStatus) {
    case MouseStatus::eActive:
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	mouseStatus = MouseStatus::eInactive;
	break;
    case MouseStatus::eInactive:
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	mouseStatus = MouseStatus::eActive;
	break;
    }
}

std::array<float, 2> Window::getMouseOffset()
{
    auto aux = mouseOffset;
    mouseOffset = std::array<float, 2>{0};
    return aux;
}

glm::vec2 Window::getMousePosition()
{
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    return glm::vec2{x, y};
}

std::vector<KeyEvent> Window::getKeyEvents()
{
    auto events = keyEvents;
    keyEvents.clear();
    return events;
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
    //TODO: this is shitty, 1 and numpad 1 are equal;
    auto key_name = glfwGetKeyName(key, scancode);
    if(key_name != NULL)
    {
	if(actions == GLFW_RELEASE)
	{
	    keyEvents.push_back(KeyEvent{
		    .keyName = key_name,
		    .keyStatus = KeyStatus::eRELEASED
		});
	}
	else
	{
	    keyEvents.push_back(KeyEvent{
		    .keyName = key_name,
		    .keyStatus = KeyStatus::ePRESSED
		});
	}
	//std::cout << key_name << ", " << scancode << ", " << key << std::endl;
    }
    else 
    {
	std::cout << " , " << scancode << ", " << key << std::endl;
    }
}

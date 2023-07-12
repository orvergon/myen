#include "myen.hpp"

#include "window.hpp"
#include "renderBackend.hpp"
#include <iostream>
#include <chrono>
#include <thread>


namespace myen {
    Myen::Myen()
    {
	auto window = new Window();
	auto renderBackend = new RenderBackend::RenderBackend(window);
	while(!window->shouldClose())
	{
	    renderBackend->drawFrame();
	    std::this_thread::sleep_for(std::chrono::milliseconds(5));
	}
    }

    Myen::~Myen()
    {}
};



#include "Window.h"

void Window::Create()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	monitors = glfwGetMonitors(&monitorCount);
	auto monitor = monitors[monitorIndex];
	auto monitorMode = glfwGetVideoMode(monitor);

	switch (mode)
	{
	case Window::Mode::Windowed:
		window = glfwCreateWindow(width, height, name, nullptr, nullptr);
		glfwSetWindowPos(window, posX, posY);

		if (maximized)
		{
			glfwMaximizeWindow(window);
		}

		glfwSetWindowAttrib(window, GLFW_MAXIMIZED, maximized);
		glfwSetWindowAttrib(window, GLFW_RESIZABLE, resizable);
		glfwSetWindowAttrib(window, GLFW_DECORATED, decorated);
		break;

	case Window::Mode::WindowedFullScreen:
		glfwWindowHint(GLFW_RED_BITS, monitorMode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, monitorMode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, monitorMode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, monitorMode->refreshRate);

		window = glfwCreateWindow(monitorMode->width, monitorMode->height, name, monitor, nullptr);
		break;

	case Window::Mode::FullScreen:
		window = glfwCreateWindow(width, height, name, monitor, nullptr);
		break;
	}

	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	glfwSetScrollCallback(window, scrollCallback);
	glfwSetWindowMaximizeCallback(window, windowMaximizeCallback);

	framebufferResized = false;
	dirty = false;
}

void Window::Update()
{
	deltaScroll = 0;

	auto newTime = std::chrono::high_resolution_clock::now();
	deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(newTime - lastTime).count();
	deltaTime /= 1000.0f;
	lastTime = newTime;

	double x;
	double y;
	glfwGetCursorPos(window, &x, &y);
	deltaMousePos = mousePos - glm::vec2(x, y);
	mousePos = glm::vec2(x, y);
	glfwPollEvents();
}

void Window::Destroy()
{
	glfwGetWindowPos(window, &posX, &posY);
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Window::UpdateFramebufferSize()
{
	framebufferResized = false;
	glfwGetFramebufferSize(window, &width, &height);
}

void Window::scrollCallback(GLFWwindow* window, double x, double y)
{
	Window::scroll += y;
	Window::deltaScroll += y;
}

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	Window::width = width;
	Window::height = height;
	Window::framebufferResized = true;
}

void Window::windowMaximizeCallback(GLFWwindow* window, int maximize)
{
	Window::maximized = maximize;
}

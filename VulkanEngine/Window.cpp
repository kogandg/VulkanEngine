#include "Window.h"

Window::Window()
{
	window = nullptr;
	monitors = nullptr;
	name = "Window";
	width = 800;
	height = 600;
	posX = 100;
	posY = 100;
	monitorIndex = 0;
	monitorCount = 0;
	framebufferResized = 0;

	deltaTime = 0.0f;

	scroll = 0.0f;
	deltaScroll = 0.0f;
	mousePos = glm::vec2(0.0f);
	deltaMousePos = glm::vec2(0.0f);

	mode = Mode::Windowed;
	dirty = true;
	resizable = true;
	decorated = true;
	maximized = false;
}

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
	
	glfwSetWindowUserPointer(window, reinterpret_cast<void*>(this));

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

GLFWwindow* Window::GetGLFWwindow()
{
	return window;
}

bool Window::IsDirty()
{
	return dirty;
}

void Window::WaitEvents()
{
	glfwWaitEvents();
}

uint32_t Window::GetWidth()
{
	return width;
}

uint32_t Window::GetHeight()
{
	return height;
}

float Window::GetDeltaTime()
{
	return deltaTime;
}

bool Window::GetShouldClose()
{
	return glfwWindowShouldClose(window);
}

float Window::GetDeltaScroll()
{
	return deltaScroll;
}

glm::vec2 Window::GetDeltaMouse()
{
	return deltaMousePos;
}

bool Window::GetFramebufferResized()
{
	return framebufferResized;
}

bool Window::IsKeyDown(uint16_t keyCode)
{
	return glfwGetKey(window, keyCode);
}

bool Window::IsMouseDown(uint16_t buttonCode)
{
	return glfwGetMouseButton(window, buttonCode);
}

void Window::scrollCallback(GLFWwindow* window, double x, double y)
{
	auto thisWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));

	thisWindow->scroll += y;
	thisWindow->deltaScroll += y;
}

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto thisWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	
	thisWindow->width = width;
	thisWindow->height = height;
	thisWindow->framebufferResized = true;
}

void Window::windowMaximizeCallback(GLFWwindow* window, int maximized)
{
	auto thisWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));

	thisWindow->maximized = maximized;
}

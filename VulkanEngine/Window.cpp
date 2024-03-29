#include "Window.h"

void Window::Create()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    monitors = glfwGetMonitors(&monitorCount);

    glfwGetVideoModes(monitors[monitorIndex], &videoModeIndex);
    videoModeIndex -= 1;

    window = glfwCreateWindow(width, height, name, nullptr, nullptr);
    glfwSetWindowPos(window, posX, posY);

    glfwSetFramebufferSizeCallback(window, Window::framebufferResizeCallback);
    glfwSetScrollCallback(window, Window::scrollCallback);
    glfwSetWindowMaximizeCallback(window, Window::windowMaximizeCallback);
    glfwSetWindowPosCallback(window, Window::windowChangePosCallback);

    dirty = false;
    Window::ApplyChanges();
}

void Window::ApplyChanges()
{
	monitors = glfwGetMonitors(&monitorCount);
	auto monitor = monitors[monitorIndex];
	auto monitorMode = glfwGetVideoMode(monitor);

    int modesCount;
    const GLFWvidmode* videoModes = glfwGetVideoModes(monitors[monitorIndex], &modesCount);
    if (videoModeIndex >= modesCount)
    {
        videoModeIndex = modesCount - 1;
    }

	switch (mode)
	{
	case Window::Mode::Windowed:
        posY = std::max(posY, 101);
        glfwSetWindowMonitor(window, nullptr, posX, posY, width, height, GLFW_DONT_CARE);

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

        glfwSetWindowMonitor(window, monitor, 0, 0, monitorMode->width, monitorMode->height, monitorMode->refreshRate);
		break;

	case Window::Mode::FullScreen:
        GLFWvidmode videoMode = videoModes[videoModeIndex];
        glfwSetWindowMonitor(window, monitor, 0, 0, videoMode.width, videoMode.height, videoMode.refreshRate);
		break;
	}

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


std::string VideoModeText(GLFWvidmode mode) 
{
    return std::to_string(mode.width) + "x" + std::to_string(mode.height) + " " + std::to_string(mode.refreshRate) + " Hz";
}

void Window::OnImgui()
{
    const auto totalSpace = ImGui::GetContentRegionAvail();
    const float totalWidth = totalSpace.x;

    if (ImGui::CollapsingHeader("Window")) 
    {
        // mode
        {
            const char* modeNames[] = { "Windowed", "Windowed FullScreen", "FullScreen" };
            ImGui::Text("Mode");
            ImGui::SameLine(totalWidth / 2.0f);
            ImGui::SetNextItemWidth(totalWidth / 2.0f);
            ImGui::PushID("modeCombo");
            if (ImGui::BeginCombo("", modeNames[(int)mode])) 
            {
                for (int i = 0; i < 3; i++) 
                {
                    bool selected = (int)mode == i;
                    if (ImGui::Selectable(modeNames[i], selected)) 
                    {
                        mode = (Window::Mode)i;
                        dirty = true;
                    }
                    if (selected) 
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopID();
        }
        if (mode != Mode::Windowed) 
        {
            // monitor
            {
                ImGui::Text("Monitor");
                ImGui::SameLine(totalWidth / 2.0f);
                ImGui::SetNextItemWidth(totalWidth / 2.0f);
                ImGui::PushID("monitorCombo");
                if (ImGui::BeginCombo("", glfwGetMonitorName(monitors[monitorIndex]))) 
                {
                    for (int i = 0; i < monitorCount; i++) 
                    {
                        bool selected = monitorIndex == i;
                        ImGui::PushID(i);
                        if (ImGui::Selectable(glfwGetMonitorName(monitors[i]), selected)) 
                        {
                            monitorIndex = i;
                            dirty = true;
                        }
                        if (selected) 
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopID();
            }
        }
        // resolution
        {
            if (mode == Mode::FullScreen) 
            {
                ImGui::Text("Resolution");
                ImGui::SameLine(totalWidth / 2.0f);
                ImGui::SetNextItemWidth(totalWidth / 4.0f);
                ImGui::PushID("monitorRes");
                int modesCount;
                const GLFWvidmode* videoModes = glfwGetVideoModes(monitors[monitorIndex], &modesCount);
                GLFWvidmode currMode = videoModes[videoModeIndex];
                std::string modeText = VideoModeText(currMode);

                if (ImGui::BeginCombo("", modeText.c_str())) 
                {
                    for (int i = 0; i < modesCount; i++) 
                    {
                        bool selected = videoModeIndex == i;
                        currMode = videoModes[i];
                        ImGui::PushID(i);
                        modeText = VideoModeText(currMode);
                        if (ImGui::Selectable(modeText.c_str(), selected)) 
                        {
                            videoModeIndex = i;
                            dirty = true;
                        }
                        if (selected) 
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopID();
            }
        }
        // windowed only
        {
            if (mode == Mode::Windowed) 
            {
                // maximized
                {
                    ImGui::Text("Maximized");
                    ImGui::SameLine(totalWidth / 2.0f);
                    ImGui::SetNextItemWidth(totalWidth / 2.0f);
                    ImGui::PushID("maximized");
                    if (ImGui::Checkbox("", &maximized)) 
                    {
                        dirty = true;
                    }
                    ImGui::PopID();
                }
                // decorated
                {
                    ImGui::Text("Decorated");
                    ImGui::SameLine(totalWidth / 2.0f);
                    ImGui::SetNextItemWidth(totalWidth / 2.0f);
                    ImGui::PushID("decorated");
                    if (ImGui::Checkbox("", &decorated)) 
                    {
                        dirty = true;
                    }
                    ImGui::PopID();
                }
                // resizable
                {
                    ImGui::Text("Resizable");
                    ImGui::SameLine(totalWidth / 2.0f);
                    ImGui::SetNextItemWidth(totalWidth / 2.0f);
                    ImGui::PushID("resizable");
                    if (ImGui::Checkbox("", &resizable)) 
                    {
                        dirty = true;
                    }
                    ImGui::PopID();
                }
            }
        }
    }
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

void Window::windowChangePosCallback(GLFWwindow* window, int x, int y)
{
    Window::posX = x;
    Window::posY = y;
}

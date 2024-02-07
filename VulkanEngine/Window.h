#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <chrono>

#include "imgui/imgui.h"

class Window
{
public:
    static void Create();
    static void Update();
    static void Destroy();
    static void UpdateFramebufferSize();

    static void OnImgui();

    static inline GLFWwindow* GetGLFWwindow() { return window; }
    static inline bool IsDirty() { return dirty; }
    static inline void WaitEvents() { glfwWaitEvents(); }
    static inline uint32_t GetWidth() { return width; }
    static inline uint32_t GetHeight() { return height; }
    static inline float GetDeltaTime() { return deltaTime; }
    static inline bool GetShouldClose() { return glfwWindowShouldClose(window) || IsKeyDown(GLFW_KEY_ESCAPE); }
    static inline float GetDeltaScroll() { return deltaScroll; }
    static inline glm::vec2 GetDeltaMouse() { return deltaMousePos; }
    static inline bool GetFramebufferResized() { return framebufferResized; }
    static inline bool IsKeyDown(uint16_t keyCode) { return glfwGetKey(window, keyCode); }
    static inline bool IsMouseDown(uint16_t buttonCode) { return glfwGetMouseButton(window, buttonCode); }

private:
    static inline GLFWwindow* window = nullptr;
    static inline GLFWmonitor** monitors = nullptr;
    static inline const char* name = "Window";
    static inline int width = 1280;
    static inline int height = 720;
    static inline int posX = 100;
    static inline int posY = 100;
    static inline int monitorIndex = 0;
    static inline int monitorCount = 0;
    static inline bool framebufferResized = false;

    static inline std::chrono::high_resolution_clock::time_point lastTime;
    static inline float deltaTime = 0.0f;

    static inline float scroll = 0.0f;
    static inline float deltaScroll = 0.0f;
    static inline glm::vec2 mousePos = glm::vec2(0.0f);
    static inline glm::vec2 deltaMousePos = glm::vec2(0.0f);

    enum class Mode 
    {
        Windowed,
        WindowedFullScreen,
        FullScreen
    };

    static inline Mode mode = Mode::Windowed;
    static inline bool dirty = true;
    static inline bool resizable = true;
    static inline bool decorated = true;
    static inline bool maximized = false;

    static void scrollCallback(GLFWwindow* window, double x, double y);
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void windowMaximizeCallback(GLFWwindow* window, int maximize);
};


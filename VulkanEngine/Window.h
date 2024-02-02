#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <chrono>

class Window
{
public:
    Window();

    void Create();
    void Update();
    void Destroy();
    void UpdateFramebufferSize();
    
    GLFWwindow* GetGLFWwindow();// { return window; }
    bool IsDirty();// { return dirty; }
    void WaitEvents();// { glfwWaitEvents(); }
    uint32_t GetWidth();// { return width; }
    uint32_t GetHeight();// { return height; }
    float GetDeltaTime();// { return deltaTime; }
    bool GetShouldClose();// { return glfwWindowShouldClose(window); }
    float GetDeltaScroll();// { return deltaScroll; }
    glm::vec2 GetDeltaMouse();// { return deltaMousePos; }
    bool GetFramebufferResized();// { return framebufferResized; }
    bool IsKeyDown(uint16_t keyCode);// { return glfwGetKey(window, keyCode); }
    bool IsMouseDown(uint16_t buttonCode);// { return glfwGetMouseButton(window, buttonCode); }

private:
    GLFWwindow* window;// = nullptr;
    GLFWmonitor** monitors;// = nullptr;
    const char* name;// = "Luz Engine";
    int width;// = 1280;
    int height;// = 720;
    int posX;// = 0;
    int posY;// = 0;
    int monitorIndex;// = 0;
    int monitorCount;// = 0;
    bool framebufferResized;// = false;

    std::chrono::high_resolution_clock::time_point lastTime;
    float deltaTime;// = .0f;

    float scroll;// = .0f;
    float deltaScroll;// = .0f;
    glm::vec2 mousePos;// = glm::vec2(.0f, .0f);
    glm::vec2 deltaMousePos;// = glm::vec2(.0f, .0f);

    enum class Mode 
    {
        Windowed,
        WindowedFullScreen,
        FullScreen
    };

    Mode mode;// = Mode::Windowed;
    bool dirty;// = true;
    bool resizable;// = true;
    bool decorated;// = true;
    bool maximized;// = true;

    static void scrollCallback(GLFWwindow* window, double x, double y);
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    static void windowMaximizeCallback(GLFWwindow* window, int maximized);
};


#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Window.h"

class Camera
{
public:
    Camera();
    void Update();
    void SetExtent(float width, float height);

    void OnImgui();

    const glm::mat4& GetView();// { return view; }
    const glm::mat4& GetProj();// { return proj; }

private:
    enum class Control 
    {
        Orbit,
        Fly
    };

    enum class Type 
    {
        Perspective,
        Orthographic
    };

    glm::mat4 view;
    glm::mat4 proj;
    glm::vec3 eye;
    glm::vec3 center;// = glm::vec3(0);
    glm::vec3 rotation;// = glm::vec3(0);
    glm::vec2 extent;// = glm::vec2(-1);

    float zoom;// = 10.0f;

    float farDistance;// = 1000.0f;
    float nearDistance;// = 0.01f;
    float horizontalFov;// = 60.0f;

    float orthoFarDistance;// = 10.0f;
    float orthoNearDistance;// = -100.0f;

    Type type = Type::Perspective;
    Control mode = Control::Orbit;

    float speed;// = 0.01;
    float zoomSpeed;// = 0.1;
    float rotationSpeed;// = 0.3;

    void updateView();
    void updateProj();

    void setControl(Control newMode);
};


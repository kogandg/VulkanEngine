#include "Camera.h"

Camera::Camera()
{
	center = glm::vec3(0);
	rotation = glm::vec3(0);
	extent = glm::vec2(-1);

	zoom = 10.0f;

	farDistance = 1000.0f;
	nearDistance = 0.01f;
	horizontalFov = 60.0f;

	orthoFarDistance = 10.0f;
	orthoNearDistance = -100.0f;

	speed = 0.01;
	zoomSpeed = 0.1;
	rotationSpeed = 0.3;

	updateView();
}

void Camera::Update()
{
	auto viewDirty = false;
	auto projDirty = false;
	glm::vec2 drag(.0f);
	glm::vec2 move(.0f);
	if (Window::IsMouseDown(GLFW_MOUSE_BUTTON_2) || Window::IsKeyDown(GLFW_KEY_LEFT_ALT))
	{
		drag = -Window::GetDeltaMouse();
	}
	if (Window::IsMouseDown(GLFW_MOUSE_BUTTON_3) || Window::IsKeyDown(GLFW_KEY_LEFT_SHIFT))
	{
		move = -Window::GetDeltaMouse();
	}

	float scroll = Window::GetDeltaScroll();
	float slowDown = Window::IsKeyDown(GLFW_KEY_LEFT_CONTROL) ? 0.1 : 1;

	switch (mode)
	{
	case Camera::Control::Orbit:
		if (drag.x != 0 || drag.y != 0)
		{
			rotation.x = std::max(-89.9f, std::min(89.9f, rotation.x + drag.y * rotationSpeed * slowDown));
			rotation.y -= drag.x * rotationSpeed * slowDown;
			viewDirty = true;
		}
		if (scroll != 0)
		{
			zoom *= std::pow(10, -scroll * zoomSpeed * slowDown);
			viewDirty = true;
			if (type == Type::Orthographic)
			{
				projDirty = true;
			}
		}
		if (move.x != 0 || move.y != 0)
		{
			glm::vec3 right = glm::vec3(view[0][0], view[1][0], view[2][0]);
			glm::vec3 up = glm::vec3(view[0][1], view[1][1], view[2][1]);
			center -= (right * move.x - up * move.y) * speed * slowDown;
			viewDirty = true;
		}
		break;

	case Camera::Control::Fly:
		glm::vec2 fpsMove(.0f, .0f);
		if (Window::IsKeyDown(GLFW_KEY_W))
		{
			fpsMove.y += 1;
		}
		if (Window::IsKeyDown(GLFW_KEY_S))
		{
			fpsMove.y -= 1;
		}
		if (Window::IsKeyDown(GLFW_KEY_D))
		{
			fpsMove.x += 1;
		}
		if (Window::IsKeyDown(GLFW_KEY_A))
		{
			fpsMove.x -= 1;
		}
		if (fpsMove.x != 0 || fpsMove.y != 0)
		{
			fpsMove = glm::normalize(fpsMove);
			glm::vec3 right = glm::vec3(view[0][0], view[1][0], view[2][0]);
			glm::vec3 front = glm::vec3(view[0][2], view[1][2], view[2][2]);
			eye += (right * fpsMove.x - front * fpsMove.y) * 0.5f * speed * slowDown * Window::GetDeltaTime();
			viewDirty = true;
		}
		if (scroll != 0)
		{
			glm::vec3 up = glm::vec3(view[0][1], view[1][1], view[2][1]);
			eye += up * scroll * speed * 25.0f * slowDown;
			viewDirty = true;
		}
		if (drag.x != 0 || drag.y != 0)
		{
			rotation += glm::vec3(drag.y, -drag.x, 0) * rotationSpeed * slowDown;
			rotation.x = std::max(-89.9f, std::min(89.9f, rotation.x));
			viewDirty = true;
		}
		break;
	}

	if (viewDirty)
	{
		updateView();
	}
	if (projDirty)
	{
		updateProj();
	}
}

void Camera::SetExtent(float width, float height)
{
	extent.x = width;
	extent.y = height;
	updateProj();
}

const glm::mat4& Camera::GetView()
{
	return view;
}

const glm::mat4& Camera::GetProj()
{
	return proj;
}

void Camera::updateView()
{
	rotation.x = std::max(-179.9f, std::min(179.9f, rotation.x));
	glm::vec3 rads;
	switch (mode)
	{
	case Control::Orbit:
		rads = glm::radians(rotation + glm::vec3(90.0f, 90.0f, 0.0f));
		glm::vec3 viewDir;
		viewDir.x = std::cos(-rads.y) * std::sin(rads.x);
		viewDir.z = std::sin(-rads.y) * std::sin(rads.x);
		viewDir.y = std::cos(rads.x);
		if (type == Type::Perspective)
		{
			viewDir *= zoom;
		}
		eye = center - viewDir;
		view = glm::lookAt(eye, center, glm::vec3(.0f, 1.0f, .0f));
		break;

	case Control::Fly:
		rads = glm::radians(rotation + glm::vec3(.0f, 180.0f, .0f));
		glm::mat4 rot(1.0f);
		rot = glm::rotate(rot, rads.z, glm::vec3(.0f, .0f, 1.0f));
		rot = glm::rotate(rot, rads.y, glm::vec3(.0f, 1.0f, .0f));
		rot = glm::rotate(rot, rads.x, glm::vec3(1.0f, .0f, .0f));
		view = glm::lookAt(eye, eye + glm::vec3(rot[2]), glm::vec3(.0f, 1.0f, .0f));
		break;
	}
}

void Camera::updateProj()
{
	switch (type)
	{
	case Camera::Type::Perspective:
		proj = glm::perspective(glm::radians(horizontalFov), extent.x / extent.y, nearDistance, farDistance);
		break;

	case Camera::Type::Orthographic:
		glm::vec2 size = glm::vec2(1.0, extent.y / extent.x) * zoom;
		proj = glm::ortho(-size.x, size.x, -size.y, size.y, orthoNearDistance, orthoFarDistance);
		break;
	}
	//glm was designed for OpenGL, where the Y coordinate of the clip coordinates is inverted
	//the easiest way to fix this is fliping the scaling factor of the y axis
	proj[1][1] *= -1;
}

void Camera::setControl(Control newMode)
{
	mode = newMode;
	if (newMode == Control::Orbit)
	{
		center = eye - zoom * glm::vec3(view[0][2], view[1][2], view[2][2]);
	}
}

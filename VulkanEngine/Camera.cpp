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

void Camera::OnImgui()
{
	const auto totalSpace = ImGui::GetContentRegionAvail();
	const float totalWidth = totalSpace.x;

	auto viewDirty = false;
	auto projDirty = false;

	if (ImGui::CollapsingHeader("Camera")) 
	{
		// type
		{
			ImGui::Text("Projection:");
			ImGui::SameLine(totalWidth / 3.0f);
			if (ImGui::RadioButton("Perspective", type == Type::Perspective)) 
			{
				type = Type::Perspective;
				projDirty = true;
				viewDirty = true;
			}
			ImGui::SameLine(2 * totalWidth / 3.0f);
			if (ImGui::RadioButton("Orthographic", type == Type::Orthographic)) 
			{
				type = Type::Orthographic;
				viewDirty = true;
				projDirty = true;
			}
		}
		// control mode
		{
			ImGui::Text("Control:");
			ImGui::SameLine(totalWidth / 3.0f);
			if (ImGui::RadioButton("Orbit", mode == Control::Orbit) && mode != Control::Orbit) 
			{
				setControl(Control::Orbit);
				viewDirty = true;
			}
			ImGui::SameLine(2 * totalWidth / 3.0f);
			if (ImGui::RadioButton("Fly", mode == Control::Fly) && mode != Control::Fly) 
			{
				setControl(Control::Fly);
				viewDirty = true;
			}
		}
		// parameters
		{
			switch (type) 
			{
			case Type::Orthographic:
				break;
			case Type::Perspective:
				ImGui::Text("Horizontal Fov");
				ImGui::SameLine(totalWidth / 3.0f);
				ImGui::PushID("fov");
				projDirty |= ImGui::DragFloat("", &horizontalFov, 1, 30, 120);
				ImGui::PopID();
				break;
			}
			ImGui::Text("Near");
			ImGui::SameLine(totalWidth / 3.0f);
			ImGui::PushID("near");
			if (type == Type::Orthographic) 
			{
				projDirty |= ImGui::DragFloat("", &orthoNearDistance, 1, -10000000, -1);
			}
			else 
			{
				projDirty |= ImGui::DragFloat("", &nearDistance, 0.001, 0.000001, 10);
			}
			ImGui::PopID();
			ImGui::Text("Far");
			ImGui::SameLine(totalWidth / 3.0f);
			ImGui::PushID("far");
			if (type == Type::Orthographic) 
			{
				projDirty |= ImGui::DragFloat("", &orthoFarDistance, 1, 1, 10000000);
			}
			else {
				projDirty |= ImGui::DragFloat("", &farDistance, 1, 1, 10000000);
			}
			ImGui::PopID();
			if (mode == Control::Fly) 
			{
				ImGui::Text("Eye");
				ImGui::SameLine(totalWidth / 3.0f);
				ImGui::PushID("eye");
				viewDirty |= ImGui::DragFloat3("", glm::value_ptr(eye));
				ImGui::PopID();
			}
			else 
			{
				ImGui::Text("Center");
				ImGui::SameLine(totalWidth / 3.0f);
				ImGui::PushID("center");
				viewDirty |= ImGui::DragFloat3("", glm::value_ptr(center));
				ImGui::PopID();
			}
			ImGui::Text("Rotation");
			ImGui::SameLine(totalWidth / 3.0f);
			ImGui::PushID("rotation");
			viewDirty |= ImGui::DragFloat3("", glm::value_ptr(rotation));
			ImGui::PopID();
			ImGui::Text("Zoom");
			ImGui::SameLine(totalWidth / 3.0f);
			ImGui::PushID("zoom");
			viewDirty |= ImGui::DragFloat("", &zoom, 0.1, 0.0001, 10000);
			ImGui::PopID();
		}
		// static parameters
		{
			if (ImGui::TreeNode("Speeds")) 
			{
				ImGui::Text("Movement");
				ImGui::SameLine(totalWidth / 3.0f);
				ImGui::PushID("mov");
				ImGui::DragFloat("", &speed, 0.0001, 0, 10.0);
				ImGui::PopID();
				ImGui::Text("Rotation");
				ImGui::SameLine(totalWidth / 3.0f);
				ImGui::PushID("rot");
				ImGui::DragFloat("", &rotationSpeed, 0.001, 0, 1);
				ImGui::PopID();
				ImGui::Text("Zoom");
				ImGui::SameLine(totalWidth / 3.0f);
				ImGui::PushID("zoom");
				ImGui::DragFloat("", &zoomSpeed, 0.001, 0, 1);
				ImGui::PopID();
				ImGui::TreePop();
			}
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

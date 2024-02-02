#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <iostream>

class Instance
{
public:
	Instance();

	void Create(GLFWwindow* window);
	void Destory();

	VkInstance GetInstance();
	VkSurfaceKHR GetSurface();
	VkAllocationCallbacks* GetAllocator();

	bool IsDirty();
	bool IsValidationLayersEnabled();

	std::vector<const char*> GetValidationLayers();

private:

	std::vector<const char*> getRequiredExtensions();
	void setupDebugMessenger();
	void createSurface(GLFWwindow* window);

	VkInstance instance;
	VkSurfaceKHR surface;
	VkAllocationCallbacks* allocator;
	VkDebugUtilsMessengerEXT debugMessenger;

	std::string applicationName;
	std::string engineName;

	uint32_t apiVersion;
	std::vector<bool> activeValidationLayers;
	std::vector<const char*> activeValidationLayersNames;
	std::vector<VkLayerProperties> validationLayers;
	std::vector<bool> activeExtensions;
	std::vector<const char*> activeExtensionsNames;
	std::vector<VkExtensionProperties> extensions;

	bool enableValidationLayers;
	bool dirty;
};


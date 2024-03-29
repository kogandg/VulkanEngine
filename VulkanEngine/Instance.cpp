#include "Instance.h"


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	//auto temp = vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = DebugCallback;
}

void Instance::Create()
{
	std::cout << "Creating Instance" << std::endl;

	static bool firstInit = true;
	if (firstInit)
	{
		firstInit = false;
		//all avalible layers
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
		validationLayers.resize(layerCount);
		activeValidationLayers.resize(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, validationLayers.data());

		//all available extensions
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, 0);
		extensions.resize(extensionCount);
		activeExtensions.resize(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		//api version
		vkEnumerateInstanceVersion(&apiVersion);

		//active default khronos validation layer
		bool khronosAvailable = false;
		for (int i = 0; i < validationLayers.size(); i++)
		{
			activeValidationLayers[i] = false;
			if (strcmp("VK_LAYER_KHRONOS_validation", validationLayers[i].layerName) == 0)
			{
				activeValidationLayers[i] = true;
				khronosAvailable = true;
				break;
			}
		}

		if (enableValidationLayers && !khronosAvailable) 
		{
			std::cerr << "Default validation layer not available!" << std::endl;
		}
	}

	//active vulkan layer
	allocator = nullptr;

	if (enableValidationLayers)
	{
		for (int i = 0; i < validationLayers.size(); i++)
		{
			if (activeValidationLayers[i])
			{
				activeValidationLayersNames.push_back(validationLayers[i].layerName);
			}
		}
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = applicationName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = engineName.c_str();
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto requiredExtensions = getRequiredExtensions();

	//set to active all extensions that we enabled
	for (size_t i = 0; i < requiredExtensions.size(); i++) 
	{
		for (size_t j = 0; j < extensions.size(); j++) 
		{
			if (strcmp(requiredExtensions[i], extensions[j].extensionName) == 0) 
			{
				activeExtensions[j] = true;
				break;
			}
		}
	}

	//get the name of all extensions that we enabled
	activeExtensionsNames.clear();
	for (size_t i = 0; i < extensions.size(); i++) 
	{
		if (activeExtensions[i]) 
		{
			activeExtensionsNames.push_back(extensions[i].extensionName);
		}
	}

	//get set all required extensions
	createInfo.enabledExtensionCount = static_cast<uint32_t>(activeExtensionsNames.size());
	createInfo.ppEnabledExtensionNames = activeExtensionsNames.data();

	//validation layers we need
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableValidationLayers) 
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(activeValidationLayersNames.size());
		createInfo.ppEnabledLayerNames = activeValidationLayersNames.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else 
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, allocator, &instance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}
	std::cout << "Created Instance" << std::endl;

	setupDebugMessenger();

	createSurface();
	std::cout << "Created surface" << std::endl;

	dirty = false;
}

void Instance::Destroy()
{
	activeValidationLayersNames.clear();
	activeExtensionsNames.clear();
	if (debugMessenger) 
	{
		DestroyDebugUtilsMessengerEXT(instance, debugMessenger, allocator);
		std::cout << "Destroyed debug messenger" << std::endl;
		debugMessenger = nullptr;
	}
	vkDestroySurfaceKHR(instance, surface, allocator);
	std::cout << "Destroyed surface" << std::endl;
	vkDestroyInstance(instance, allocator);
	std::cout << "Destroyed instance" << std::endl;
}

void Instance::OnImgui()
{
	if (ImGui::CollapsingHeader("Instance")) 
	{
		const auto totalSpace = ImGui::GetContentRegionAvail();
		const float totalWidth = totalSpace.x;

		ImGui::Text("API Version");
		ImGui::SameLine(totalWidth / 2);
		ImGui::PushID("apiVersion");
		ImGui::Text("%d", apiVersion);
		ImGui::PopID();
		ImGui::Text("Application");
		ImGui::SameLine(totalWidth / 2);
		ImGui::PushID("appName");
		ImGui::InputText("", &applicationName[0], 256);
		ImGui::PopID();
		ImGui::Text("Engine");
		ImGui::SameLine(totalWidth / 2);
		ImGui::PushID("engineName");
		ImGui::InputText("", &engineName[0], 256);
		ImGui::PopID();
		ImGui::Text("Enable Validation Layers");
		ImGui::SameLine(totalWidth / 2);
		ImGui::PushID("enableValidation");
		ImGui::Checkbox("", &enableValidationLayers);
		ImGui::PopID();
		ImGui::Separator();
		if (ImGui::TreeNode("Validation Layers")) 
		{
			for (size_t i = 0; i < validationLayers.size(); i++) {
				ImGui::PushID(i);
				bool active = activeValidationLayers[i];
				ImGui::Checkbox("", &active);
				activeValidationLayers[i] = active;
				ImGui::SameLine();
				const auto& layer = validationLayers[i];
				if (ImGui::TreeNode(layer.layerName)) {
					// description
					ImGui::Dummy(ImVec2(5.0f, .0f));
					ImGui::SameLine();
					ImGui::Text("Description: %s", layer.description);
					// spec version
					ImGui::Dummy(ImVec2(5.0f, .0f));
					ImGui::SameLine();
					ImGui::Text("Spec Version: %d", layer.specVersion);
					// impl version
					ImGui::Dummy(ImVec2(5.0f, .0f));
					ImGui::SameLine();
					ImGui::Text("Implementation Version: %d", layer.implementationVersion);
					ImGui::TreePop();
				}
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
		ImGui::Separator();
		if (ImGui::TreeNode("Extensions")) 
		{
			for (size_t i = 0; i < extensions.size(); i++) {
				ImGui::PushID(i);
				bool active = activeExtensions[i];
				ImGui::Checkbox("", &active);
				activeExtensions[i] = active;
				ImGui::PopID();
				ImGui::SameLine();
				const auto& ext = extensions[i];
				if (ImGui::TreeNode(ext.extensionName)) {
					ImGui::Dummy(ImVec2(5.0f, .0f));
					ImGui::SameLine();
					ImGui::Text("Spec Version: %d", ext.specVersion);
					ImGui::TreePop();
				}
			}
			ImGui::TreePop();
		}
		ImGui::Separator();
		if (ImGui::Button("Apply Changes")) 
		{
			dirty = true;
		}
	}
	if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_I)) 
	{
		dirty = true;
	}
}

std::vector<const char*> Instance::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		//requiredExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return requiredExtensions;
}

void Instance::setupDebugMessenger()
{
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, allocator, &debugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void Instance::createSurface()
{
	if (glfwCreateWindowSurface(instance, Window::GetGLFWwindow(), allocator, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}
}

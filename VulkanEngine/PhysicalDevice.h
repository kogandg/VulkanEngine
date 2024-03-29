#pragma once

#include <vulkan/vulkan.h>
#include <set>
#include <vector>

#include "Instance.h"

class PhysicalDevice
{
public:
	static void Create();
    static void Destroy();
    static void OnSurfaceUpdate();
    static void UpdateDevice();
    static uint32_t FindMemoryType(uint32_t type, VkMemoryPropertyFlags properties);
    static bool SupportFormat(VkFormat format, VkImageTiling tiling, VkFormatFeatureFlags features);

    static void OnImgui();

    static inline bool IsDirty() { return dirty; }
    static inline VkPhysicalDevice GetVkPhysicalDevice() { return device->vkDevice; }
    static inline VkPhysicalDeviceFeatures GetFeatures() { return device->features; }
    static inline uint32_t GetPresentFamily() { return device->presentFamily; }
    static inline uint32_t GetGraphicsFamily() { return device->graphicsFamily; }
    static inline VkSampleCountFlags GetSampleCounts() { return device->sampleCounts; }
    static inline VkSampleCountFlagBits GetMaxSamples() { return device->maxSamples; }
    static inline VkSurfaceCapabilitiesKHR GetCapabilities() { return device->capabilities; }
    static inline const std::vector<const char*>& GetRequiredExtensions() { return requiredExtensions; }
    static inline const std::vector<VkPresentModeKHR>& GetPresentModes() { return device->presentModes; }
    static inline const std::vector<VkSurfaceFormatKHR>& GetSurfaceFormats() { return device->surfaceFormats; }
    static inline const std::vector<VkExtensionProperties>& GetExtensions() { return device->extensions; }

private:
    static inline std::vector<PhysicalDevice> allDevices;
    static inline PhysicalDevice* device = nullptr;
    static inline std::vector<const char*> requiredExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    static inline int index = -1;
    static inline bool dirty = true;

    bool suitable = false;
    int presentFamily = -1;
    int graphicsFamily = -1;

    VkPhysicalDevice vkDevice = VK_NULL_HANDLE;
    VkSampleCountFlagBits maxSamples = VK_SAMPLE_COUNT_1_BIT;
    VkSampleCountFlags sampleCounts;

    VkPhysicalDeviceFeatures features{};
    VkSurfaceCapabilitiesKHR capabilities{};
    VkPhysicalDeviceProperties properties{};
    VkPhysicalDeviceMemoryProperties memoryProperties{};

    std::vector<VkPresentModeKHR> presentModes;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkExtensionProperties> extensions;
    std::vector<VkQueueFamilyProperties> families;
};


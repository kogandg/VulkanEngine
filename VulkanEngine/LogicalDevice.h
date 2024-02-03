#pragma once

#include <vulkan/vulkan.h>

#include "Instance.h"
#include "PhysicalDevice.h"

class LogicalDevice
{
public:
    static void Create();
    static void Destroy();

    static inline VkDevice GetVkDevice() { return device; }
    static inline VkQueue GetPresentQueue() { return presentQueue; }
    static inline VkQueue GetGraphicsQueue() { return graphicsQueue; }
    static inline bool IsDirty() { return dirty; }

private:
    static inline VkDevice device = VK_NULL_HANDLE;
    static inline VkQueue presentQueue = VK_NULL_HANDLE;
    static inline VkQueue graphicsQueue = VK_NULL_HANDLE;
    static inline bool dirty = true;

};


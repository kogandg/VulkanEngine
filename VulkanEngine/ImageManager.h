#pragma once
#include <vulkan/vulkan.h>

#include "BufferManager.h"

struct ImageResource 
{
    VkImage image;
    VkImageView view;
    VkDeviceMemory memory;
};

struct ImageDesc 
{
    VkFormat format;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    VkSampleCountFlagBits numSamples;
    VkImageUsageFlags usage;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkImageAspectFlags aspect;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkDeviceSize size;

    uint32_t width;
    uint32_t height;
    uint32_t mipLevels = 1;
};

class ImageManager 
{
public:
    static void Create(const ImageDesc& desc, ImageResource& resource);
    static void Create(const ImageDesc& desc, ImageResource& resource, BufferResource& buffer);
    static void Destroy(ImageResource& resource);
};

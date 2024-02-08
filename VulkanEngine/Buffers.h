#pragma once

#include <vulkan/vulkan.h>

#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "Instance.h"

struct BufferDescriptor
{
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkMemoryPropertyFlags properties;
};

struct BufferResource 
{
    VkBuffer buffer;
    VkDeviceMemory memory;
};

class Buffers 
{
public:
    static void Create(const BufferDescriptor& desc, BufferResource& resource);
    static void CreateStaged(const BufferDescriptor& desc, BufferResource& resource, void* data);
    static void Destroy(BufferResource& resource);

    static void Copy(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    static void Update(BufferResource& resource, void* data, VkDeviceSize size);

    static void CreateStagingBuffer(BufferResource& resource, void* data, VkDeviceSize size);
    static void CreateIndexBuffer(BufferResource& resource, void* data, VkDeviceSize size);
    static void CreateVertexBuffer(BufferResource& resource, void* data, VkDeviceSize size);
};


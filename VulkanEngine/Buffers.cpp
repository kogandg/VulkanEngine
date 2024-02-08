#include "Buffers.h"

void Buffers::Create(const BufferDescriptor& desc, BufferResource& resource)
{
    auto device = LogicalDevice::GetVkDevice();
    auto allocator = Instance::GetAllocator();

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = desc.size;
    bufferInfo.usage = desc.usage;
    // only from the graphics queue
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    auto result = vkCreateBuffer(device, &bufferInfo, allocator, &resource.buffer);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer!");
    }

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, resource.buffer, &memReq);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = PhysicalDevice::FindMemoryType(memReq.memoryTypeBits, desc.properties);

    result = vkAllocateMemory(device, &allocInfo, allocator, &resource.memory);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, resource.buffer, resource.memory, 0);
}

void Buffers::CreateStaged(const BufferDescriptor& desc, BufferResource& resource, void* data)
{
    BufferResource staging;
    Buffers::Create(desc, resource);
    CreateStagingBuffer(staging, data, desc.size);
    Buffers::Copy(staging.buffer, resource.buffer, desc.size);
    Buffers::Destroy(staging);
}

void Buffers::Destroy(BufferResource& resource)
{
    vkDestroyBuffer(LogicalDevice::GetVkDevice(), resource.buffer, Instance::GetAllocator());
    vkFreeMemory(LogicalDevice::GetVkDevice(), resource.memory, Instance::GetAllocator());
}

void Buffers::Copy(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
    auto commandBuffer = LogicalDevice::BeginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);

    LogicalDevice::EndSingleTimeCommands(commandBuffer);
}

void Buffers::Update(BufferResource& resource, void* data, VkDeviceSize size)
{
    void* dst;
    vkMapMemory(LogicalDevice::GetVkDevice(), resource.memory, 0, size, 0, &dst);
    memcpy(dst, data, size);
    vkUnmapMemory(LogicalDevice::GetVkDevice(), resource.memory);
}

void Buffers::CreateStagingBuffer(BufferResource& resource, void* data, VkDeviceSize size)
{
    BufferDescriptor stagingDesc;
    stagingDesc.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingDesc.properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    stagingDesc.size = size;
    Buffers::Create(stagingDesc, resource);
    Buffers::Update(resource, data, size);
}

void Buffers::CreateIndexBuffer(BufferResource& resource, void* data, VkDeviceSize size)
{
    BufferDescriptor desc;
    desc.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    desc.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.size = size;
    Buffers::CreateStaged(desc, resource, data);
}

void Buffers::CreateVertexBuffer(BufferResource& resource, void* data, VkDeviceSize size)
{
    BufferDescriptor desc;
    desc.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    desc.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc.size = size;
    Buffers::CreateStaged(desc, resource, data);
}

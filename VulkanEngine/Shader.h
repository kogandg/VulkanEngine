#pragma once
#include <vulkan/vulkan.h>
#include <vector>

#include "Instance.h"
#include "LogicalDevice.h"

#include <spirv_reflect.h>

struct ShaderDescriptor 
{
    std::vector<char> shaderBytes;
    VkShaderStageFlagBits stageBit;
};

struct ShaderResource 
{
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    VkPipelineShaderStageCreateInfo stageCreateInfo{};
};

class Shader 
{
public:
    static void Create(const ShaderDescriptor& desc, ShaderResource& resource);
    static void Destroy(ShaderResource& resource);
};


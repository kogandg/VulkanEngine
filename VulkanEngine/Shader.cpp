#include "Shader.h"

void Shader::Create(const ShaderDescriptor& desc, ShaderResource& resource)
{
    auto device = LogicalDevice::GetVkDevice();
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = desc.shaderBytes.size();
    createInfo.pCode = (const uint32_t*)(desc.shaderBytes.data());
    auto result = vkCreateShaderModule(device, &createInfo, Instance::GetAllocator(), &resource.shaderModule);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module!");
    }

    resource.stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    resource.stageCreateInfo.stage = desc.stageBit;
    resource.stageCreateInfo.module = resource.shaderModule;
    resource.stageCreateInfo.pName = "main";
    resource.stageCreateInfo.pSpecializationInfo = nullptr;
}

void Shader::Destroy(ShaderResource& resource)
{
    vkDestroyShaderModule(LogicalDevice::GetVkDevice(), resource.shaderModule, Instance::GetAllocator());
}

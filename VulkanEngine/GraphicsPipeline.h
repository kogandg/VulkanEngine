#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include "Shader.h"

#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "SwapChain.h"
#include "Instance.h"
#include "VulkanUtils.h"

struct GraphicsPipelineDescriptor
{
    std::string name = "Default";
    std::vector<ShaderDescriptor> shaderStages;
    VkVertexInputBindingDescription bindingDesc{};
    std::vector<VkVertexInputAttributeDescription> attributesDesc;
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    VkPipelineMultisampleStateCreateInfo multisampling{};
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    VkPipelineColorBlendStateCreateInfo colorBlendState{};
    std::vector<VkDescriptorSetLayoutBinding> bindings;
};

struct GraphicsPipelineResource
{
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    bool dirty = false;
};

class GraphicsPipeline
{
public:
    static void Create(const GraphicsPipelineDescriptor& desc, GraphicsPipelineResource& resource);
    static void Destroy(GraphicsPipelineResource& resource);
    static void OnImgui(GraphicsPipelineDescriptor& desc, GraphicsPipelineResource& resource);
};


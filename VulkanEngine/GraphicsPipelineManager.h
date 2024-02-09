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
    VkDescriptorSetLayout sceneDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout modelDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout textureDescriptorSetLayout = VK_NULL_HANDLE;
    bool dirty = false;
};

class GraphicsPipelineManager
{
public:
    static void Create();
    static void Destroy();
    static void CreatePipeline(const GraphicsPipelineDescriptor& desc, GraphicsPipelineResource& resource);
    static void DestroyPipeline(GraphicsPipelineResource& resource);
    static void OnImgui(GraphicsPipelineDescriptor& desc, GraphicsPipelineResource& resource);

    static inline VkDescriptorPool GetDescriptorPool() { return descriptorPool; }

private:
    static inline VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
};


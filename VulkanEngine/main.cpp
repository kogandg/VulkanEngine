#include "PhysicalDevice.h"
#include "LogicalDevice.h"
#include "Instance.h"
#include "ImageManager.h"
#include "Window.h"
#include "SwapChain.h"
#include "Camera.h"
#include "Shader.h"
#include "FileManager.h"
#include "GraphicsPipelineManager.h"
#include "UnlitGraphicsPipeline.h"
#include "BufferManager.h"
#include "SceneManager.h"
#include "TextureManager.h"
#include "AssetManager.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <algorithm>
#include <fstream>
#include <array>
#include <chrono>
#include <unordered_map>

#include <stb_image.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <glm/gtx/quaternion.hpp>


#include "imgui/imgui.h"

#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"
#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif

#include "imgui/ImGuizmo.h"

//const std::string MODEL_PATH = "Converse.obj";
const std::string TEXTURE_PATH = "assets/checker.png";

void CheckVulkanResult(VkResult res) 
{
    if (res == 0) 
    {
        return;
    }
    std::cerr << "vulkan error during some imgui operation: " << res << '\n';
    if (res < 0) 
    {
        throw std::runtime_error("");
    }
}

class HelloTriangleApplication
{
public:
	void run()
	{
        Setup();
        Create();
        MainLoop();
        Destroy();
	}

private:
	Camera camera;

    SceneUBO sceneUBO;

	// image 
	uint32_t mipLevels;
	VkImage textureImage;
	VkSampler textureSampler;
	VkImageView textureImageView;
	VkDeviceMemory textureImageMemory;
    VkDescriptorSet textureDescriptor;

    TextureResource* textureResource;

    //imgui
    ImDrawData* imguiDrawData = nullptr;

    void Setup()
    {
        UnlitGraphicsPipeline::Setup();
        SceneManager::Setup();
    }

    void Create()
    {
        SetupImgui();

        Window::Create();

        Instance::Create();
        PhysicalDevice::Create();
        LogicalDevice::Create();
        SwapChain::Create();

        std::cout << "Finish creating SwapChain" << std::endl;

        GraphicsPipelineManager::Create();
        UnlitGraphicsPipeline::Create();

        createTextureImage();
        createTextureImageView();
        createTextureSampler();
        createTextureDescriptor();

        std::cout << "Finish loading model" << std::endl;
        
        CreateImgui();

        createUniformProjection();

        std::cout << "Finish initializing Vulkan" << std::endl;

        SceneManager::Create();
	}

    void createTextureDescriptor() 
    {
        auto device = LogicalDevice::GetVkDevice();
        auto allocator = Instance::GetAllocator();
        auto unlitGPO = UnlitGraphicsPipeline::GetResource();

        textureResource = AssetManager::LoadImageFile(TEXTURE_PATH);

        std::vector<VkDescriptorSetLayout> layouts(1, unlitGPO.textureDescriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = GraphicsPipelineManager::GetDescriptorPool();
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = layouts.data();

        auto vkRes = vkAllocateDescriptorSets(device, &allocInfo, &textureDescriptor);
        if (vkRes != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate scene descriptor sets!");
        }

        std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureResource->image.view;
        imageInfo.sampler = textureResource->sampler;

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = textureDescriptor;
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(LogicalDevice::GetVkDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }

    void Destroy() 
    {
        DestroyVulkan();
        FinishImgui();
    }

    void DestroyVulkan() 
    {
        SceneManager::Destroy();
        MeshManager::Destroy();
        TextureManager::Destroy();

        auto device = LogicalDevice::GetVkDevice();
        auto instance = Instance::GetInstance();

        DestroySwapChain();

        vkDestroySampler(device, textureSampler, nullptr);
        vkDestroyImageView(device, textureImageView, nullptr);
        vkDestroyImage(device, textureImage, nullptr);
        vkFreeMemory(device, textureImageMemory, nullptr);
        
        LogicalDevice::Destroy();
        PhysicalDevice::Destroy();
        Instance::Destroy();
        
        Window::Destroy();
    }

    void DestroySwapChain() 
    {
        UnlitGraphicsPipeline::Destroy();
        GraphicsPipelineManager::Destroy();

        DestroyImgui();

        SwapChain::Destroy();
        std::cout << "Destroyed SwapChain" << std::endl;
    }

    void MainLoop() 
    {
        auto instance = Instance::GetInstance();
        while (!Window::GetShouldClose()) 
        {
            Window::Update();
            camera.Update();
            drawFrame();
            if (Instance::IsDirty() || PhysicalDevice::IsDirty() || Window::IsDirty()) 
            {
                vkDeviceWaitIdle(LogicalDevice::GetVkDevice());
                Destroy();
                Create();
            }
        }
        vkDeviceWaitIdle(LogicalDevice::GetVkDevice());
    }

    void imguiDrawFrame()
    {
        auto device = LogicalDevice::GetVkDevice();
        auto instance = Instance::GetInstance();
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("Vulkan")) 
        {
            Window::OnImgui();
            Instance::OnImgui();
            PhysicalDevice::OnImgui();
            LogicalDevice::OnImgui();
            SwapChain::OnImgui();
            UnlitGraphicsPipeline::OnImgui();
            camera.OnImgui();
        }
        ImGui::End();

        SceneManager::OnImgui();

        ImGuizmo::BeginFrame();
        static ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::ROTATE;
        static ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;

       /* if (ImGui::Begin("Transform")) 
        {
            if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_1)) 
            {
                currentGizmoOperation = ImGuizmo::TRANSLATE;
            }
            if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_2))
            {
                currentGizmoOperation = ImGuizmo::ROTATE;
            }
            if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_3))
            {
                currentGizmoOperation = ImGuizmo::SCALE;
            }
            if (ImGui::RadioButton("Translate", currentGizmoOperation == ImGuizmo::TRANSLATE)) 
            {
                currentGizmoOperation = ImGuizmo::TRANSLATE;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Rotate", currentGizmoOperation == ImGuizmo::ROTATE)) 
            {
                currentGizmoOperation = ImGuizmo::ROTATE;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Scale", currentGizmoOperation == ImGuizmo::SCALE)) 
            {
                currentGizmoOperation = ImGuizmo::SCALE;
            }

            ImGui::InputFloat3("Position", glm::value_ptr(transform.position));
            ImGui::InputFloat3("Rotation", glm::value_ptr(transform.rotation));
            ImGui::InputFloat3("Scale", glm::value_ptr(transform.scale));
            ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(transform.position), glm::value_ptr(transform.rotation),
                glm::value_ptr(transform.scale), glm::value_ptr(ubo.model));*/

        Model* selectedModel = SceneManager::GetSelectedModel();

        if (selectedModel != nullptr)
        {
            if (ImGui::Begin("Transform"))
            {
                Transform& transform = selectedModel->transform;
                ModelUBO& modelUBO = selectedModel->ubo;

                if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_1))
                {
                    currentGizmoOperation = ImGuizmo::TRANSLATE;
                }
                if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_2))
                {
                    currentGizmoOperation = ImGuizmo::ROTATE;
                }
                if (ImGui::IsKeyPressed((ImGuiKey)GLFW_KEY_3))
                {
                    currentGizmoOperation = ImGuizmo::SCALE;
                }
                if (ImGui::RadioButton("Translate", currentGizmoOperation == ImGuizmo::TRANSLATE))
                {
                    currentGizmoOperation = ImGuizmo::TRANSLATE;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Rotate", currentGizmoOperation == ImGuizmo::ROTATE)) {
                    currentGizmoOperation = ImGuizmo::ROTATE;

                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Scale", currentGizmoOperation == ImGuizmo::SCALE))
                {
                    currentGizmoOperation = ImGuizmo::SCALE;
                }

                ImGui::InputFloat3("Position", glm::value_ptr(transform.position));
                ImGui::InputFloat3("Rotation", glm::value_ptr(transform.rotation));
                ImGui::InputFloat3("Scale", glm::value_ptr(transform.scale));
                ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(transform.position), glm::value_ptr(transform.rotation), glm::value_ptr(transform.scale), glm::value_ptr(modelUBO.model));

                if (currentGizmoOperation != ImGuizmo::SCALE) 
                {
                    if (ImGui::RadioButton("Local", currentGizmoMode == ImGuizmo::LOCAL)) 
                    {
                        currentGizmoMode = ImGuizmo::LOCAL;
                    }
                    ImGui::SameLine();
                    if (ImGui::RadioButton("World", currentGizmoMode == ImGuizmo::WORLD)) 
                    {
                        currentGizmoMode = ImGuizmo::WORLD;
                    }
                }
                else 
                {
                    currentGizmoMode = ImGuizmo::LOCAL;
                }
                glm::mat4 guizmoProj(sceneUBO.proj);
                guizmoProj[1][1] *= -1;

                ImGuiIO& io = ImGui::GetIO();
                ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
                ImGuizmo::Manipulate(glm::value_ptr(sceneUBO.view), glm::value_ptr(guizmoProj), currentGizmoOperation, currentGizmoMode, glm::value_ptr(modelUBO.model), nullptr, nullptr);
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(modelUBO.model), glm::value_ptr(transform.position), glm::value_ptr(transform.rotation), glm::value_ptr(transform.scale));
            }
            ImGui::End();
        }
            /*if (currentGizmoOperation != ImGuizmo::SCALE) 
            {
                if (ImGui::RadioButton("Local", currentGizmoMode == ImGuizmo::LOCAL)) 
                {
                    currentGizmoMode = ImGuizmo::LOCAL;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("World", currentGizmoMode == ImGuizmo::WORLD)) 
                {
                    currentGizmoMode = ImGuizmo::WORLD;
                }
            }
            else 
            {
                currentGizmoMode = ImGuizmo::LOCAL;
            }*/

            /*glm::mat4 guizmoProj(ubo.proj);
            guizmoProj[1][1] *= -1;

            ImGuiIO& io = ImGui::GetIO();
            ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
            ImGuizmo::Manipulate(glm::value_ptr(ubo.view), glm::value_ptr(guizmoProj), currentGizmoOperation,
                currentGizmoMode, glm::value_ptr(ubo.model), nullptr, nullptr);
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(ubo.model), glm::value_ptr(transform.position),
                glm::value_ptr(transform.rotation), glm::value_ptr(transform.scale));
        }
        ImGui::End();*/

        ImGui::Render();
        imguiDrawData = ImGui::GetDrawData();

    }

    void updateCommandBuffer(size_t frameIndex) 
    {
        auto device = LogicalDevice::GetVkDevice();
        auto instance = Instance::GetInstance();
        auto commandBuffer = SwapChain::GetCommandBuffer(frameIndex);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = SwapChain::GetRenderPass();
        renderPassInfo.framebuffer = SwapChain::GetFramebuffer(frameIndex);

        renderPassInfo.renderArea.offset = { 0, 0 };
        renderPassInfo.renderArea.extent = SwapChain::GetExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        auto sceneDescriptor = SceneManager::GetSceneDescriptor(frameIndex);
        auto unlitGPR = UnlitGraphicsPipeline::GetResource();
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, unlitGPR.pipeline);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, unlitGPR.layout, 0, 1, &sceneDescriptor, 0, nullptr);

        for (const Model* model : SceneManager::GetModels())
        {
            if (model->mesh != nullptr) 
            {
                MeshResource* mesh = model->mesh;
                VkBuffer vertexBuffers[] = { mesh->vertexBuffer.buffer };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(commandBuffer, mesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, unlitGPR.layout, 1, 1, &model->descriptors[frameIndex], 0, nullptr);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, unlitGPR.layout, 2, 1, &model->materialDescriptors[frameIndex], 0, nullptr);
                vkCmdDrawIndexed(commandBuffer, mesh->indexCount, 1, 0, 0, 0);
            }
        }

        //imgui draw
        ImGui_ImplVulkan_RenderDrawData(imguiDrawData, commandBuffer);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to record command buffer!");
        }

    }

    void drawFrame() 
    {
        auto device = LogicalDevice::GetVkDevice();
        auto instance = Instance::GetInstance();
     
        imguiDrawFrame();

        auto image = SwapChain::Acquire();
        
        updateUniformBuffer(image);
        updateCommandBuffer(image);

        SwapChain::SubmitAndPresent(image);
        
        if (dirtyFrameResources())
        {
            recreateFrameResources();
            return;
        }
    }

    bool dirtyFrameResources()
    {
        return SwapChain::IsDirty() || Window::GetFramebufferResized() || UnlitGraphicsPipeline::IsDirty();
    }

    void recreateFrameResources()
    {
        auto device = LogicalDevice::GetVkDevice();
        auto instance = Instance::GetInstance();

        vkDeviceWaitIdle(device);

        if (Window::GetFramebufferResized() || SwapChain::IsDirty())
        {
            // busy wait while the window is minimized
            while (Window::GetWidth() == 0 || Window::GetHeight() == 0) 
            {
                Window::WaitEvents();
            }
            Window::UpdateFramebufferSize();
            vkDeviceWaitIdle(device);
            DestroySwapChain();
            PhysicalDevice::OnSurfaceUpdate();
            SwapChain::Create();
            GraphicsPipelineManager::Create();
            UnlitGraphicsPipeline::Create();
            SceneManager::RecreateDescriptors();
            createTextureDescriptor();
            CreateImgui();
            createUniformProjection();
        }
        else if (UnlitGraphicsPipeline::IsDirty()) 
        {
            UnlitGraphicsPipeline::Destroy();
            UnlitGraphicsPipeline::Create();
        }
    }

    void createUniformProjection() 
    {
        auto ext = SwapChain::GetExtent();
        camera.SetExtent(ext.width, ext.height);
    }

    void updateUniformBuffer(uint32_t currentImage) 
    {
        for (Model* model : SceneManager::GetModels()) 
        {
            BufferManager::Update(model->buffers[currentImage], &model->ubo, sizeof(model->ubo));
        }

        sceneUBO.view = camera.GetView();
        sceneUBO.proj = camera.GetProj();
        BufferManager::Update(SceneManager::GetUniformBuffer(currentImage), &sceneUBO, sizeof(sceneUBO));
    }

    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) 
    {
        auto device = LogicalDevice::GetVkDevice();
        auto instance = Instance::GetInstance();
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        // tiling cannot be changed later, if we want to acces the directly
        // access the texels of this image, we should use LINEAR
        imageInfo.tiling = tiling;
        // not usable by the GPU, the first transition will discard the texels
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        // sampled defines that the image will be accessed by the shader
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = numSamples;
        imageInfo.flags = 0;

        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = PhysicalDevice::FindMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(device, image, imageMemory, 0);
    }

    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) 
    {
        auto device = LogicalDevice::GetVkDevice();
        auto instance = Instance::GetInstance();

        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(PhysicalDevice::GetVkPhysicalDevice(), imageFormat, &formatProperties);
        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) 
        {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        VkCommandBuffer commandBuffer = LogicalDevice::BeginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) 
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            if (mipWidth > 1) 
            {
                mipWidth /= 2;
            }
            if (mipHeight > 1) 
            {
                mipHeight /= 2;
            }
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        LogicalDevice::EndSingleTimeCommands(commandBuffer);
    }

    void createTextureImage() 
    {
        auto device = LogicalDevice::GetVkDevice();
        auto instance = Instance::GetInstance();
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;
        auto colorFormat = VK_FORMAT_R8G8B8A8_UNORM;

        if (!pixels) 
        {
            throw std::runtime_error("failed to load texture image!");
        }

        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        BufferResource staging;
        BufferManager::CreateStagingBuffer(staging, pixels, imageSize);

        stbi_image_free(pixels);

        createImage(texWidth, texHeight, mipLevels, VK_SAMPLE_COUNT_1_BIT, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

        transitionImageLayout(textureImage, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
        copyBufferToImage(staging.buffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

        generateMipmaps(textureImage, colorFormat, texWidth, texHeight, mipLevels);

        BufferManager::Destroy(staging);
    }

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) 
    {
        VkCommandBuffer commandBuffer = LogicalDevice::BeginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        // if we were transferring queue family ownership
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) 
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        LogicalDevice::EndSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) 
    {
        auto device = LogicalDevice::GetVkDevice();
        auto instance = Instance::GetInstance();
        VkCommandBuffer commandBuffer = LogicalDevice::BeginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        LogicalDevice::EndSingleTimeCommands(commandBuffer);
    }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) 
    {
        auto device = LogicalDevice::GetVkDevice();
        auto instance = Instance::GetInstance();
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        // what the purpose of the image and how it will be accessed
        // our images will be used as color targets without any mipmapping
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }

    void createTextureImageView() 
    {
        textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
    }

    void createTextureSampler() 
    {
        auto device = LogicalDevice::GetVkDevice();
        auto instance = Instance::GetInstance();
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;

        // get the max ansotropy level of my device
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(PhysicalDevice::GetVkPhysicalDevice(), &properties);

        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

        // what color to return when clamp is active in addressing mode
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        // if comparison is enabled, texels will be compared to a value an the result 
        // is used in filtering operations, can be used in PCF on shadow maps
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(mipLevels);

        if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) 
        {
            throw std::runtime_error("failed to create texture sampler!");
        }
    }

    void SetupImgui() 
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        ImGui::StyleColorsDark();
    }

    void CreateImgui() 
    {
        auto device = LogicalDevice::GetVkDevice();
        auto instance = Instance::GetInstance();

        auto alloc = Instance::GetAllocator();

        ImGui_ImplGlfw_InitForVulkan(Window::GetGLFWwindow(), true);

        ImGui_ImplVulkan_InitInfo initInfo{};
        initInfo.Instance = instance;
        initInfo.PhysicalDevice = PhysicalDevice::GetVkPhysicalDevice();
        initInfo.Device = device;
        initInfo.QueueFamily = PhysicalDevice::GetGraphicsFamily();
        initInfo.Queue = LogicalDevice::GetGraphicsQueue();
        initInfo.PipelineCache = VK_NULL_HANDLE;
        initInfo.DescriptorPool = GraphicsPipelineManager::GetDescriptorPool();
        initInfo.MinImageCount = 2;
        initInfo.ImageCount = (uint32_t)SwapChain::GetNumFrames();
        initInfo.MSAASamples = SwapChain::GetNumSamples();
        initInfo.Allocator = Instance::GetAllocator();
        initInfo.CheckVkResultFn = CheckVulkanResult;
        ImGui_ImplVulkan_Init(&initInfo, SwapChain::GetRenderPass());

        auto commandBuffer = LogicalDevice::BeginSingleTimeCommands();
        ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
        LogicalDevice::EndSingleTimeCommands(commandBuffer);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void DestroyImgui() 
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
    }

    void FinishImgui() 
    {
        ImGui::DestroyContext();
    }

};

int main()
{
	HelloTriangleApplication app;

	try
	{
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
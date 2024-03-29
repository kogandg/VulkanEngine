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
        Finish();
	}

private:
	Camera camera;

    SceneUBO sceneUBO;

    //imgui
    ImDrawData* imguiDrawData = nullptr;

    void Setup()
    {
        UnlitGraphicsPipeline::Setup();
        TextureManager::Setup();
        SetupImgui();
    }

    void Create()
    {
        CreateVulkan();
        SceneManager::Setup();
    }

    void CreateVulkan()
    {
        Window::Create();

        Instance::Create();
        PhysicalDevice::Create();
        LogicalDevice::Create();
        SwapChain::Create();

        std::cout << "Finish creating SwapChain" << std::endl;

        GraphicsPipelineManager::Create();
        UnlitGraphicsPipeline::Create();

        std::cout << "Finish loading model" << std::endl;
        
        CreateImgui();

        createUniformProjection();

        std::cout << "Finish initializing Vulkan" << std::endl;

        TextureManager::Create();
        MeshManager::Create();
        SceneManager::Create();
	}

    void Finish() 
    {
        DestroyVulkan();
        SceneManager::Finish();
        MeshManager::Finish();
        TextureManager::Finish();
        FinishImgui();
    }

    void DestroyVulkan() 
    {
        auto device = LogicalDevice::GetVkDevice();
        auto instance = Instance::GetInstance();

        DestroyFrameResources();
        
        MeshManager::Destroy();
        TextureManager::Destroy();
        LogicalDevice::Destroy();
        PhysicalDevice::Destroy();
        Instance::Destroy();
        
        Window::Destroy();
    }

    void DestroyFrameResources()
    {
        SceneManager::Destroy();
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
            if (DirtyGlobalResources()) 
            {
                vkDeviceWaitIdle(LogicalDevice::GetVkDevice());
                DestroyVulkan();
                CreateVulkan();
            }
            else if (DirtyFrameResources())
            {
                RecreateFrameResources();
            }
            else if (UnlitGraphicsPipeline::IsDirty())
            {
                vkDeviceWaitIdle(LogicalDevice::GetVkDevice());
                UnlitGraphicsPipeline::Destroy();
                UnlitGraphicsPipeline::Create();
            }
            else if (Window::IsDirty())
            {
                Window::ApplyChanges();
            }
        }
        vkDeviceWaitIdle(LogicalDevice::GetVkDevice());
    }

    bool DirtyGlobalResources() 
    {
        bool dirty = false;
        dirty |= Instance::IsDirty();
        dirty |= PhysicalDevice::IsDirty();
        dirty |= LogicalDevice::IsDirty();
        return dirty;
    }

    bool DirtyFrameResources() 
    {
        bool dirty = false;
        dirty |= SwapChain::IsDirty();
        dirty |= Window::GetFramebufferResized();
        return dirty;
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

        if (SwapChain::IsDirty())
        {
            return;
        }
        
        updateUniformBuffer(image);
        updateCommandBuffer(image);

        SwapChain::SubmitAndPresent(image);
    }

    void RecreateFrameResources()
    {
        auto device = LogicalDevice::GetVkDevice();
        auto instance = Instance::GetInstance();

        vkDeviceWaitIdle(device);

        while (Window::GetWidth() == 0 || Window::GetHeight() == 0) 
        {
            Window::WaitEvents();
        }

        Window::UpdateFramebufferSize();
        vkDeviceWaitIdle(device);
        DestroyFrameResources();
        PhysicalDevice::OnSurfaceUpdate();
        SwapChain::Create();
        GraphicsPipelineManager::Create();
        UnlitGraphicsPipeline::Create();
        SceneManager::Create();
        CreateImgui();
        createUniformProjection();
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
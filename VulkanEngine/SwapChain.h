#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include "Image.h"
#include "LogicalDevice.h"
#include "PhysicalDevice.h"
#include "Window.h"
#include "Instance.h"

class SwapChain
{
public:
    static void Create();
    static void Destroy();
     
    static inline bool IsDirty() { return dirty; }
    static inline VkExtent2D GetExtent() { return extent; }
    static inline uint32_t GetNumFrames() { return images.size(); }
    static inline uint32_t GetFramesInFlight() { return framesInFlight; }
    static inline VkRenderPass GetRenderPass() { return renderPass; }
    static inline VkSwapchainKHR GetVkSwapChain() { return swapChain; }
    static inline VkSampleCountFlagBits GetNumSamples() { return numSamples; }
    static inline VkFramebuffer GetFramebuffer(size_t i) { return framebuffers[i]; }

private:
    static inline VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    static inline VkRenderPass renderPass = VK_NULL_HANDLE;
    static inline std::vector<VkImage> images;
    static inline std::vector<VkImageView> views;
    static inline std::vector<VkFramebuffer> framebuffers;
            
    static inline ImageResource colorRes;
    static inline ImageResource depthRes;
            
    static inline uint32_t additionalImages;
    static inline uint32_t framesInFlight;
    static inline VkFormat depthFormat;
    static inline VkExtent2D extent;
    static inline uint32_t currentFrame = 0;
    static inline int newAdditionalImages = 1;
    static inline int newFramesInFlight= 3;
    static inline bool dirty = true;
            
    // preferred, warn if not available
    static inline VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    static inline VkColorSpaceKHR colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    static inline VkPresentModeKHR presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
    static inline VkSampleCountFlagBits numSamples = VK_SAMPLE_COUNT_64_BIT;
     
    static VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    static VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& presentModes);
    static VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);


};


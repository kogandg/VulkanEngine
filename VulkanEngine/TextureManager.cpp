#include "TextureManager.h"

#include "AssetManager.h"

void TextureManager::Create()
{
    TextureResource* newTexture = AssetManager::LoadImageFile(defaultTexture->path);
    defaultTexture->image = newTexture->image;
    defaultTexture->sampler = newTexture->sampler;
    if (textures[textures.size() - 1] != newTexture)
    {
        throw std::runtime_error("New texture is different than last texture on buffer!");
    }
    
    textures.pop_back();
    for (TextureResource* texture : textures) 
    {
        newTexture = AssetManager::LoadImageFile(texture->path);
        texture->image = newTexture->image;
        texture->sampler = newTexture->sampler;
        if (textures[textures.size() - 1] != newTexture)
        {
            throw std::runtime_error("New texture is different than last texture on buffer!");
        }
        textures.pop_back();
    }
}

void TextureManager::Setup() 
{
    defaultTexture = new TextureResource();
    defaultTexture->path = "assets/checker.png";
}

void TextureManager::Finish()
{
    for (TextureResource* texture : textures) 
    {
        delete texture;
    }
    textures.clear();
}

void TextureManager::Destroy()
{
    ImageManager::Destroy(defaultTexture->image);
    vkDestroySampler(LogicalDevice::GetVkDevice(), defaultTexture->sampler, Instance::GetAllocator());

    for (TextureResource* texture : textures) 
    {
        ImageManager::Destroy(texture->image);
        vkDestroySampler(LogicalDevice::GetVkDevice(), texture->sampler, Instance::GetAllocator());
    }
}


TextureResource* TextureManager::CreateTexture(TextureDescriptor& desc)
{
    auto device = LogicalDevice::GetVkDevice();
    auto instance = Instance::GetInstance();

    ImageDesc imageDesc{};
    imageDesc.numSamples = VK_SAMPLE_COUNT_1_BIT;
    imageDesc.width = desc.width;
    imageDesc.height = desc.height;
    imageDesc.mipLevels = (uint32_t)(std::floor(std::log2(std::max(desc.width, desc.height)))) + 1;
    imageDesc.numSamples = VK_SAMPLE_COUNT_1_BIT;
    imageDesc.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageDesc.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageDesc.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    imageDesc.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageDesc.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageDesc.properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    imageDesc.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    imageDesc.size = (uint32_t)(desc.width * desc.height * 4);

    TextureResource* res = new TextureResource();

    BufferResource staging;
    BufferManager::CreateStagingBuffer(staging, desc.data, imageDesc.size);
    ImageManager::Create(imageDesc, res->image, staging);
    BufferManager::Destroy(staging);

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
    samplerInfo.maxLod = static_cast<float>(imageDesc.mipLevels);

    auto vkRes = vkCreateSampler(device, &samplerInfo, nullptr, &res->sampler);
    if (vkRes != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create texture sampler!");
    }

    textures.push_back(res);

    return res;
}

#pragma once

#include <string>
#include <filesystem>

#include "ImageManager.h"

struct TextureDescriptor
{
    void* data;
    uint32_t width;
    uint32_t height;
};

struct TextureResource 
{
    std::filesystem::path path;
    ImageResource image;
    VkSampler sampler;
};

class TextureManager 
{
public:
    static void Create();
    static void Setup();
    static void Finish();
    static void Destroy();
    static TextureResource* CreateTexture(TextureDescriptor& desc);

    static inline TextureResource* GetDefaultTexture() { return defaultTexture; }

private:
    static inline std::vector<TextureResource*> textures;
    static inline TextureResource* defaultTexture;
};

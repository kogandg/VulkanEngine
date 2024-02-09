#pragma once

#include <string>

#include "ImageManager.h"

struct TextureDescriptor
{
    void* data;
    uint32_t width;
    uint32_t height;
};

struct TextureResource 
{
    std::string   name;
    ImageResource image;
    VkSampler     sampler;
};

class TextureManager 
{
public:
    static void Create();
    static void Destroy();
    static TextureResource* CreateTexture(TextureDescriptor& desc);

private:
    static inline std::vector<TextureResource*> textures;
};

#pragma once

#include <string>

#include "BufferManager.h"
#include "MeshManager.h"
#include "TextureManager.h"
#include "Transform.h"

struct ModelUBO 
{
    glm::mat4 model = glm::mat4(1.0f);
};

struct Model
{
    std::string name;
    Transform transform;
    MeshResource* mesh = nullptr;
    TextureResource* texture = nullptr;
    ModelUBO ubo;
    std::vector<VkDescriptorSet> descriptors;
    std::vector<BufferResource> buffers;
    std::vector<VkDescriptorSet> materialDescriptors;
};
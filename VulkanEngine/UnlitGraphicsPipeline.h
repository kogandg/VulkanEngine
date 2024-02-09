#pragma once

#include "GraphicsPipelineManager.h"

#include "MeshManager.h"
#include "UnlitGraphicsPipeline.h"
#include "FileManager.h"
#include "SwapChain.h"

class UnlitGraphicsPipeline
{
public:
    static void Setup();
    static void Create();
    static void Destroy();
    static void OnImgui();

    static inline bool IsDirty() { return resource.dirty; }
    static inline GraphicsPipelineResource& GetResource() { return resource; }

private:
    static inline GraphicsPipelineDescriptor desc{};
    static inline GraphicsPipelineResource resource{};
};


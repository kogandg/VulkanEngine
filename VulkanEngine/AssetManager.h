#pragma once

#include <filesystem>

#include "Model.h"
#include "MeshManager.h"
#include "TextureManager.h"

class AssetManager 
{
public:
    static void Create();
    static void Destroy();
    static void Load(std::filesystem::path path);
    static std::vector<Model*> LoadObjFile(std::filesystem::path path);
    static TextureResource* LoadImageFile(std::filesystem::path path);
};

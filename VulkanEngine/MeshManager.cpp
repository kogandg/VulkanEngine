#include "MeshManager.h"

#include <tiny_obj_loader.h>

void MeshManager::Create()
{
    if (meshes.size() != descs.size())
    {
        throw std::runtime_error("Number of mesh descs is different than number of meshes!");
    }
    for (int i = 0; i < meshes.size(); i++) 
    {
        SetupMesh(descs[i], meshes[i]);
    }
}

void MeshManager::Destroy()
{
    for (MeshResource* mesh : meshes)
    {
        BufferManager::Destroy(mesh->vertexBuffer);
        BufferManager::Destroy(mesh->indexBuffer);
    }
}

void MeshManager::Finish()
{
    for (MeshDescriptor* desc : descs) 
    {
        delete desc;
    }
    for (MeshResource* mesh : meshes) 
    {
        delete mesh;
    }
    meshes.clear();
}

MeshResource* MeshManager::CreateMesh(MeshDescriptor* desc)
{
    MeshResource* mesh = new MeshResource();
    MeshManager::SetupMesh(desc, mesh);
    meshes.push_back(mesh);
    descs.push_back(desc);
    return mesh;
}

void MeshManager::SetupMesh(MeshDescriptor* desc, MeshResource* resource)
{
    resource->indexCount = desc->indices.size();
    BufferManager::CreateVertexBuffer(resource->vertexBuffer, desc->vertices.data(), sizeof(desc->vertices[0]) * desc->vertices.size());
    BufferManager::CreateIndexBuffer(resource->indexBuffer, desc->indices.data(), sizeof(desc->indices[0]) * desc->indices.size());
}

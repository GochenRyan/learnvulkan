#include <DeferredRendering/Model.h>
#include <format>
#include <iostream>

void Texture::loadImage(std::string_view path, vk::raii::Device *device, const vk::raii::Queue &queue)
{
}

void Texture::updateDescriptor()
{
}

bool Model::loadFromFile(std::string filename, vk::raii::Device &device, uint32_t nodeIndex, const vk::raii::Queue &queue, uint32_t fileLoadingFlags, float scale)
{
    // Create manager & iosettings
    FbxManager* manager = FbxManager::Create();
    if (!manager) return false;
    FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(ios);

    // Create an importer
    FbxImporter* importer = FbxImporter::Create(manager, "");
    bool importOK = importer->Initialize(filename.c_str(), -1, manager->GetIOSettings());
    if (!importOK) {
        std::cerr << "FBX Importer init failed: " << importer->GetStatus().GetErrorString() << "\n";
        importer->Destroy();
        manager->Destroy();
        return false;
    }

    // Create a scene and import the file
    FbxScene* scene = FbxScene::Create(manager, "scene");
    if (!importer->Import(scene)) {
        std::cerr << "FBX import failed: " << importer->GetStatus().GetErrorString() << "\n";
        importer->Destroy();
        manager->Destroy();
        return false;
    }
    importer->Destroy();

    // Triangulation Scenario
    FbxGeometryConverter geoConverter(manager);
    if (!geoConverter.Triangulate(scene, /*replace*/ true)) {
        std::cerr << "Warning: Triangulate failed or returned false\n";
    }

    // Convert to the right-hand axis system
    FbxAxisSystem desiredAxis = FbxAxisSystem::OpenGL;
    FbxAxisSystem sceneAxis = scene->GetGlobalSettings().GetAxisSystem();
    if (sceneAxis != desiredAxis) {
        desiredAxis.ConvertScene(scene);
    }

    FbxNode* root = scene->GetRootNode();
    if (!root) {
        std::cerr << "Empty scene\n";
        scene->Destroy();
        manager->Destroy();
        return false;
    }

    return false;
}

void Model::loadImages(FbxNode* node)
{
    if (!node) return;
    int matCount = node->GetMaterialCount();
    for (int mi = 0; mi < matCount; ++mi)
    {
        FbxSurfaceMaterial* mat = node->GetMaterial(mi);
        if (!mat) continue;

        FbxProperty currentProperty = mat->GetFirstProperty();
        while (currentProperty.IsValid())
        {
            uint32_t unsupportedTexCnt = currentProperty.GetSrcObjectCount<FbxLayeredTexture>() + currentProperty.GetSrcObjectCount<FbxProceduralTexture>();
            if (unsupportedTexCnt > 0)
            {
                std::cerr << std::format("Unsupported texture count: {0}\n", unsupportedTexCnt);
                return;
            }

            uint32_t supportedTexCnt = currentProperty.GetSrcObjectCount<FbxFileTexture>();

            for (uint32_t texIndex = 0; texIndex < supportedTexCnt; ++texIndex)
            {
                FbxFileTexture* pFileTex = currentProperty.GetSrcObject<FbxFileTexture>();
                if (!pFileTex)
                    continue;

                const char* pFBXPropertyName = currentProperty.GetNameAsCStr();
                const auto& iterPropertyName = FBXPropertyToNew.find(pFBXPropertyName);
            }

            currentProperty = mat->GetNextProperty(currentProperty);
        }
    }
}
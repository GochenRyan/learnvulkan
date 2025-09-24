#include <DeferredRendering/Model.h>
#include <format>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static std::string toLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) { return std::tolower(c); });
    return r;
}

static std::string getExtension(const std::string& path) {
    size_t p = path.find_last_of('.');
    if (p == std::string::npos) return "";
    return toLower(path.substr(p + 1));
}

void Texture::loadImage(std::string_view path, vk::raii::Device *device, const vk::raii::Queue &queue)
{
    this->device = device;

    bool isKtx = false;
    // Image points to an external ktx file
    if (path.find_last_of(".") != std::string::npos) {
        if (path.substr(path.find_last_of(".") + 1) == "ktx") {
            isKtx = true;
        }
    }

    if (!isKtx)
    {
        int texWidth, texHeight, texChannels;
        std::string texturePath = ASSETS_SRC_DIR "/Model/kris-light-world-form-deltarune/textures/krish_light_form_text.png";
        stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels,
            // The STBI_rgb_alpha value forces the image to be loaded with an alpha channel, even if it doesn’t have one
            STBI_rgb_alpha);
        
        if (!pixels)
            throw std::runtime_error(std::format("failed to load texture image : {0}", path));

        vk::DeviceSize imageSize = texWidth * texHeight * 4;
        vk::raii::DeviceMemory stagingBufferMemory = nullptr;
        mipLevels = static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1.0);

        //todo: Device Context -- divece, physic device
    }

    // Sampler
    vk::SamplerCreateInfo samplerCI{
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,
        .mipmapMode = vk::SamplerMipmapMode::eLinear,
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,
        .mipLodBias = 0,
        .anisotropyEnable = vk::True,
        //todo: a good way to get maxSamplerAnisotropy
        //.maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .maxAnisotropy = 8.0f,
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,
        .maxLod = (float)mipLevels,
        .borderColor = vk::BorderColor::eFloatOpaqueWhite,
        /*
            The unnormalizedCoordinates field specifies which coordinate system you want to use to address texels in an image.
            If this field is VK_TRUE, then you can simply use coordinates within the [0, texWidth) and [0, texHeight) range. If it is VK_FALSE, then the texels are addressed using the [0, 1) range on all axes.
            Real-world applications almost always use normalized coordinates, because then it’s possible to use textures of varying resolutions with the exact same coordinates.
        */
        .unnormalizedCoordinates = vk::False
    };
    sampler = device->createSampler(samplerCI);
}

void Texture::updateDescriptor()
{
}

bool Model::loadFromFile(std::string filename, vk::raii::Device &device, const vk::raii::Queue &transferQueue, FileLoadingFlags fileLoadingFlags, float scale)
{
    this->device = &device;

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

    const int nodeCount = root->GetChildCount();
    std::vector<FbxNode*> nodes;
    nodes.reserve(nodeCount);

    std::function<void(FbxNode*)> collectNodes = [&](FbxNode* n) {
        nodes.push_back(n);
        for (int i = 0; i < n->GetChildCount(); ++i)
            collectNodes(n->GetChild(i));
        };
    collectNodes(root);

    for (FbxNode* node : nodes)
    {
        loadImages(node, transferQueue);
    }

    return false;
}

void Model::loadImages(FbxNode* node, const vk::raii::Queue& transferQueue)
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

                std::string path = pFileTex->GetFileName();
                Texture texture;
                texture.loadImage(path, device, transferQueue);
            }

            currentProperty = mat->GetNextProperty(currentProperty);
        }
    }
}
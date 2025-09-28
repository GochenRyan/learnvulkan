#include <DeferredRendering/Model.h>

#include <format>
#include <iostream>
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

void Texture::loadImage(std::string_view path, VulkanDevice* device, const vk::raii::Queue &queue)
{
    this->deviceVK = device;

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
        stbi_uc* pixels = stbi_load(path.data(), &texWidth, &texHeight, &texChannels,
            // The STBI_rgb_alpha value forces the image to be loaded with an alpha channel, even if it doesn't have one
            STBI_rgb_alpha);
        
        if (!pixels)
            throw std::runtime_error(std::format("failed to load texture image : {0}", path));

        vk::DeviceSize imageSize = texWidth * texHeight * 4;
        mipLevels = static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1.0);

        vk::raii::Buffer stagingBuffer = nullptr;
        vk::raii::DeviceMemory stagingBufferMemory = nullptr;
        deviceVK->CreateBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
        void* dataStaging = stagingBufferMemory.mapMemory(0, imageSize);
        memcpy(dataStaging, pixels, imageSize);
        stagingBufferMemory.unmapMemory();

        stbi_image_free(pixels);

        deviceVK->CreateImage(texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, mipLevels, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, image, deviceMemory);
        deviceVK->transitionImageLayout(image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, queue);
        deviceVK->copyBufferToImage(stagingBuffer, image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), queue);
        deviceVK->transitionImageLayout(image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, queue);

        // Generate the mip chain
        std::unique_ptr<vk::raii::CommandBuffer> blitCmd = deviceVK->beginSingleTimeCommands();
        for (uint32_t i = 1; i < mipLevels; i++)
        {
            vk::ImageBlit imageBlit{
                .srcSubresource = {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .mipLevel = i - 1,
                    .layerCount = 1
                },
                .dstSubresource = {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .mipLevel = i,
                    .layerCount = 1
                }
            };
            imageBlit.srcOffsets[1] = { int32_t(width >> (i - 1)), int32_t(height >> (i - 1)), 1 };
            imageBlit.srcOffsets[1] = { int32_t(width >> i), int32_t(height >> i), 1 };

            vk::ImageSubresourceRange mipSubRange{
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = i,
                .levelCount = 1,
                .layerCount = 1
            };
            {
                vk::ImageMemoryBarrier imageMemoryBarrier{
                    .srcAccessMask = vk::AccessFlagBits::eNone,
                    .dstAccessMask = vk::AccessFlagBits::eTransferWrite,
                    .oldLayout = vk::ImageLayout::eUndefined,
                    .newLayout = vk::ImageLayout::eTransferDstOptimal,
                    .image = image,
                    .subresourceRange = mipSubRange
                };
                blitCmd->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, imageMemoryBarrier);   
            }
            blitCmd->blitImage(image, vk::ImageLayout::eTransferSrcOptimal, /* blitting between different levels of the same image */image, vk::ImageLayout::eTransferDstOptimal, imageBlit, vk::Filter::eLinear);
            {
                vk::ImageMemoryBarrier imageMemoryBarrier{
                    .srcAccessMask = vk::AccessFlagBits::eTransferWrite,
                    .dstAccessMask = vk::AccessFlagBits::eTransferRead,
                    .oldLayout = vk::ImageLayout::eTransferDstOptimal,
                    .newLayout = vk::ImageLayout::eTransferSrcOptimal,
                    .image = image,
                    .subresourceRange = mipSubRange
                };
                blitCmd->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, imageMemoryBarrier);
            }
        }
        
        vk::ImageMemoryBarrier barrier{
            .oldLayout = vk::ImageLayout::eTransferSrcOptimal,
            .newLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .image = image,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = mipLevels,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        blitCmd->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

        deviceVK->endSingleTimeCommands(*blitCmd, queue);
    }
    else
    {
        //todo: ktx
    }

    vk::PhysicalDeviceProperties properties = deviceVK->physicalDevice.getProperties();
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
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,
        .maxLod = (float)mipLevels,
        .borderColor = vk::BorderColor::eFloatOpaqueWhite,
        /*
            The unnormalizedCoordinates field specifies which coordinate system you want to use to address texels in an image.
            If this field is VK_TRUE, then you can simply use coordinates within the [0, texWidth) and [0, texHeight) range. If it is VK_FALSE, then the texels are addressed using the [0, 1) range on all axes.
            Real-world applications almost always use normalized coordinates, because then it�s possible to use textures of varying resolutions with the exact same coordinates.
        */
        .unnormalizedCoordinates = vk::False
    };
    sampler = deviceVK->logicDevice.createSampler(samplerCI);

    vk::ImageViewCreateInfo viewCI{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = vk::Format::eR8G8B8A8Srgb,
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = mipLevels, .layerCount = 1 }
    };
    deviceVK->logicDevice.createImageView(viewCI);

    imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    descriptor.sampler = sampler;
    descriptor.imageView = view;
    descriptor.imageLayout = imageLayout;
}

Texture::~Texture()
{
    deviceVK = nullptr;
}

void Texture::updateDescriptor()
{
    descriptor.sampler = sampler;
    descriptor.imageView = view;
    descriptor.imageLayout = imageLayout;
}

bool Model::loadFromFile(std::string filename, VulkanDevice* device, const vk::raii::Queue &transferQueue, FileLoadingFlags fileLoadingFlags, float scale)
{
    this->deviceVK = device;

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
        loadMaterials(node, transferQueue);
    }

    return false;
}

void Model::loadMaterials(FbxNode* node, const vk::raii::Queue& transferQueue)
{
    if (!node) return;
    int matCount = node->GetMaterialCount();
    for (int mi = 0; mi < matCount; ++mi)
    {
        FbxSurfaceMaterial* mat = node->GetMaterial(mi);
        if (!mat) continue;

        Material material(deviceVK);
        FbxProperty currentProperty = mat->GetFirstProperty();
        while (currentProperty.IsValid())
        {
            const char* pFBXPropertyName = currentProperty.GetNameAsCStr();

            if (const auto& iter = FBXPropertyToNew.find(pFBXPropertyName); iter != FBXPropertyToNew.cend())
            {
                Texture texture;
                texture.loadImage(path, deviceVK, transferQueue);
                texture.index = static_cast<uint32_t>(textureLookup.size());
                textureLookup.push_back(std::move(texture));

                material.textureMap[iter->second] = texture.index;
                materialLookup.push_back(material);
            }
            else
            {
                throw std::runtime_error(std::format("Can not find material: {0}", pFBXPropertyName));
            }

            currentProperty = mat->GetNextProperty(currentProperty);
        }
    }
}

void Model::loadNodeRecursively(FbxNode* node)
{
    const auto* pNodeAttribute = node->GetNodeAttribute();

    if (pNodeAttribute && pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
    {
    }
}
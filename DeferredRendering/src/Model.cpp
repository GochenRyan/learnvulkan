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

        width = texWidth;
        height = texHeight;

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

        layerCount = 1;

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

    format = vk::Format::eR8G8B8A8Srgb;

    vk::ImageViewCreateInfo viewCI{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = vk::Format::eR8G8B8A8Srgb,
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = mipLevels, .layerCount = 1 }
    };
    view = deviceVK->logicDevice.createImageView(viewCI);

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


void Primitive::SetDimensions(glm::vec3 min, glm::vec3 max)
{
}


Mesh::Mesh(VulkanDevice *deviceVK, glm::mat4 matrix)
{
}

Mesh::~Mesh()
{
}


glm::mat4 Node::LocalMatrix()
{
    return glm::mat4();
}

glm::mat4 Node::GetMatrix()
{
    return glm::mat4();
}

void Node::Update()
{
}

Node::~Node()
{
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
        for (size_t i = 0; i < n->GetChildCount(); ++i)
            collectNodes(n->GetChild(i));
        };
    collectNodes(root);

    for (FbxNode* node : nodes)
    {
        loadMaterials(node, transferQueue);
    }

    loadNodeRecursively(root);

    return false;
}

void Model::loadMaterials(FbxNode* node, const vk::raii::Queue& transferQueue)
{
    if (!node) return;
    int matCount = node->GetMaterialCount();
    for (size_t mi = 0; mi < matCount; ++mi)
    {
        FbxSurfaceMaterial* mat = node->GetMaterial(mi);
        if (!mat) continue;

        auto& material = materialLookup.emplace_back(deviceVK);
        FbxProperty currentProperty = mat->GetFirstProperty();
        while (currentProperty.IsValid())
        {
            const char* pFBXPropertyName = currentProperty.GetNameAsCStr();

            if (const auto& iter = FBXPropertyToNew.find(pFBXPropertyName); iter != FBXPropertyToNew.cend())
            {
                FbxFileTexture* resTexture = nullptr;

                const int lLayeredTextureCount = currentProperty.GetSrcObjectCount<FbxLayeredTexture>();
                if (lLayeredTextureCount > 0)
                {
                    FbxLayeredTexture* lLayeredTexture = currentProperty.GetSrcObject<FbxLayeredTexture>();

                    const int lNbTextures = lLayeredTexture->GetSrcObjectCount<FbxFileTexture>();
                    if (lNbTextures > 0)
                    {
                        resTexture = lLayeredTexture->GetSrcObject<FbxFileTexture>();
                    }
                }
                else
                {
                    //Get first texture connected to property. Anyway, there shouldn't be more than one.
                    FbxFileTexture* lTexture = currentProperty.GetSrcObject<FbxFileTexture>();
                    if (lTexture)
                        resTexture = lTexture;
                }

                if (resTexture != nullptr)
                {
                    Texture& texture = textureLookup.emplace_back();
                    texture.path = resTexture->GetFileName();
                    texture.name = resTexture->GetRelativeFileName();
                    //todo: prepare vk context
                    texture.loadImage(texture.path, deviceVK, transferQueue);
                    texture.index = static_cast<uint32_t>(textureLookup.size());

                    material.textureMap[iter->second] = texture.index;
                }
            }
            else if (FloatParamArray.find(pFBXPropertyName) != FloatParamArray.cend())
            {
                double val = currentProperty.Get<double>();
                material.FloatParamMap[pFBXPropertyName] = static_cast<float>(val);
            }
            else
            {
                //std::cout << std::format("missing property name : {0}", pFBXPropertyName) << std::endl;
            }

            currentProperty = mat->GetNextProperty(currentProperty);
        }
    }
}

void Model::loadNodeRecursively(FbxNode* fbxNode)
{
    const auto* pNodeAttribute = fbxNode->GetNodeAttribute();

    if (pNodeAttribute && pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
    {
        auto& node = nodeLookup.emplace_back();
        node.name = fbxNode->GetName();

        FbxMesh* mesh = fbxNode->GetMesh();
        // Control points = positions
        FbxVector4* controlPoints = mesh->GetControlPoints();
        const int polygonCount = mesh->GetPolygonCount();
        const int vertexCount = mesh->GetControlPointsCount();

        size_t beginIndex = vertexLookup.size();
        vertexLookup.resize(beginIndex + polygonCount * 3);

        std::vector<uint32_t> polygonSizes(polygonCount, 0);
        int polygonIndexCount = 0;
        for (size_t p = 0; p < polygonCount; ++p)
        {
            // Expect polySize = 3
            const int polySize = mesh->GetPolygonSize(p);
            polygonSizes[p] = polySize;
            for (size_t v = 0; v < polySize; ++v)
            {
                // Position
                const int controlPointIndex = mesh->GetPolygonVertex(p, v);
                auto& vert = vertexLookup[beginIndex + polygonIndexCount + v];
                FbxVector4 cp = controlPoints[controlPointIndex];
                vert.pos = glm::vec3(static_cast<float>(cp[0]), static_cast<float>(cp[1]), static_cast<float>(cp[2]));
            }
            polygonIndexCount += polySize;
        }

        auto* const mainLayer = mesh->GetLayer(0);
        const int* indices = mesh->GetPolygonVertices();

        // Normal
        auto* fbxNorms = mainLayer->GetNormals();
        std::vector<glm::vec3> norms(polygonIndexCount, glm::zero<glm::vec3>());
        GetFBXAttributeValue(fbxNorms, norms, indices, polygonIndexCount, polygonSizes, polygonCount, vertexCount, glm::zero<glm::vec3>());
        
        for (size_t i = 0; i < polygonCount; ++i)
        {
            auto& vert = vertexLookup[beginIndex + i];
            vert.normal = norms[i];
        }

        // Tangent
        // TBN = [T, cross(N, T)*sign, N]
        std::vector<glm::vec3> tangents(polygonIndexCount, glm::zero<glm::vec3>());
        std::vector<glm::vec3> binormals(polygonIndexCount, glm::zero<glm::vec3>());
        auto* fbxTangents = mainLayer->GetTangents();
        auto* fbxBinormals = mainLayer->GetBinormals();
        if (fbxTangents && fbxBinormals)
        {
            GetFBXAttributeValue(fbxTangents, tangents, indices, polygonIndexCount, polygonSizes, polygonCount, vertexCount, glm::zero<glm::vec3>());
            GetFBXAttributeValue(fbxBinormals, binormals, indices, polygonIndexCount, polygonSizes, polygonCount, vertexCount, glm::zero<glm::vec3>());
            for (size_t i = 0; i < tangents.size(); ++i)
            {
                const auto tangent = tangents[i];
                auto binormal = glm::cross(norms[i], tangent);
                float sign = glm::dot(binormal, binormals[i]);

                auto& vert = vertexLookup[beginIndex + i];
                vert.tangent = glm::vec4(tangent, sign > 0 ? 1.f : -1.f);
            }
        }

        // UV
        std::vector<glm::vec2> uvs[MAX_UV_SETS];
        int uvsetIndex = 0;
        for (size_t i = 0; i < mesh->GetLayerCount(); i++)
        {
            FbxLayer* fbxLayer = mesh->GetLayer(i);
            if (!fbxLayer)
                continue;
            FbxLayerElementUV* fbxUVs = fbxLayer->GetUVs();
            if (!fbxUVs)
                continue;
        
            uvs[uvsetIndex].resize(polygonIndexCount);
            GetFBXAttributeValue(fbxUVs, uvs[uvsetIndex], indices, polygonIndexCount, polygonSizes, polygonCount, vertexCount, glm::zero<glm::vec2>());
            for (size_t j = 0; j < polygonCount; ++j)
            {
                auto& vert = vertexLookup[beginIndex + i];
                vert.uvs[uvsetIndex] = uvs[uvsetIndex][j];
                if (flipV)
                {
                    vert.uvs[uvsetIndex].y = 1.0f - vert.uvs[uvsetIndex].y;
                }
            }

            uvsetIndex++;
            if (uvsetIndex == MAX_UV_SETS)
                break;
        }

        // Vertex Color
        auto* fbxVertexColors = mainLayer->GetVertexColors();
        if (fbxVertexColors)
        {
            std::vector<glm::vec4> vertexColors(polygonIndexCount, glm::zero<glm::vec4>());
            GetFBXAttributeValue(fbxVertexColors, vertexColors, indices, polygonIndexCount, polygonSizes, polygonCount, vertexCount, glm::zero<glm::vec4>());
            for (size_t i = 0; i < polygonCount; ++i)
            {
                auto& vert = vertexLookup[beginIndex + i];
                vert.color = vertexColors[i];
            }
        }
    }
    else
    {
        for (size_t i = 0; i < fbxNode->GetChildCount(); ++i)
        {
            loadNodeRecursively(fbxNode->GetChild(i));
        }
    }
}
#include <DeferredRendering/Model.h>

#include <format>
#include <iostream>
#include <stb_image.h>
#include <unordered_set>

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
            imageBlit.dstOffsets[1] = { int32_t(width >> i) > 0 ? int32_t(width >> i) : 1, int32_t(height >> i) > 0 ? int32_t(height >> i) : 1, 1 };

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
    updateDescriptor();
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


void Model::createDescriptorSet(vk::raii::DescriptorPool& descriptorPool, vk::raii::DescriptorSetLayout& descriptorSetLayout)
{
    for (auto& material : materialLookup)
    {
        vk::DescriptorSetAllocateInfo allocInfo{
            .descriptorPool = descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &*descriptorSetLayout
        };
        material.descriptorSet = std::move(deviceVK->logicDevice.allocateDescriptorSets(allocInfo)[0]);

        std::vector<vk::WriteDescriptorSet> writeDescriptorSets;

        {
            auto& texture = textureLookup[material.textureMap["BaseColor"]];
            vk::WriteDescriptorSet descriptorWrite{
                .dstSet = material.descriptorSet,
                .dstBinding = static_cast<uint32_t>(writeDescriptorSets.size()),
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &(texture.descriptor)
            };
            writeDescriptorSets.push_back(descriptorWrite);
        }

        {
            auto& texture = textureLookup[material.textureMap["Normal"]];
            vk::WriteDescriptorSet descriptorWrite{
                .dstSet = material.descriptorSet,
                .dstBinding = static_cast<uint32_t>(writeDescriptorSets.size()),
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &(texture.descriptor)
            };
            writeDescriptorSets.push_back(descriptorWrite);
        }
        
        {
            auto& texture = textureLookup[material.textureMap["Metallic"]];
            vk::WriteDescriptorSet descriptorWrite{
                .dstSet = material.descriptorSet,
                .dstBinding = static_cast<uint32_t>(writeDescriptorSets.size()),
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &(texture.descriptor)
            };
            writeDescriptorSets.push_back(descriptorWrite);
        }

        {
            auto& texture = textureLookup[material.textureMap["Roughness"]];
            vk::WriteDescriptorSet descriptorWrite{
                .dstSet = material.descriptorSet,
                .dstBinding = static_cast<uint32_t>(writeDescriptorSets.size()),
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &(texture.descriptor)
            };
            writeDescriptorSets.push_back(descriptorWrite);
        }

        deviceVK->logicDevice.updateDescriptorSets(writeDescriptorSets, {});
    }
}


void Primitive::SetDimensions(glm::vec3 min, glm::vec3 max)
{
}


Mesh::Mesh(VulkanDevice *deviceVK, glm::mat4 matrix)
{
}

Mesh::~Mesh()
{
    deviceVK = nullptr;
    uniformBuffer.mapped = nullptr;
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

Model::~Model()
{
    deviceVK = nullptr;
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

    loadNodeRecursively(root, nullptr);

    size_t vertexBufferSize = vertexLookup.size() * sizeof(Vertex);
    size_t indexBufferSize = indexLookup.size() * sizeof(uint32_t);

    assert((vertexBufferSize > 0) && "vertex buffer size = 0");
    assert((indexBufferSize > 0) && "index buffer size = 0");

    struct StageingBuffer
    {
        vk::raii::Buffer buffer = nullptr;
        vk::raii::DeviceMemory memory = nullptr;
    } vertexStaging{}, indexStaging{};

    deviceVK->CreateBuffer(vertexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vertexStaging.buffer, vertexStaging.memory, vertexLookup.data());
    deviceVK->CreateBuffer(indexBufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, indexStaging.buffer, indexStaging.memory, indexLookup.data());

    deviceVK->CreateBuffer(vertexBufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal, vertices.buffer, vertices.memory);
    deviceVK->CreateBuffer(indexBufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, indices.buffer, indices.memory);

    deviceVK->copyBuffer(vertexStaging.buffer, vertices.buffer, vertexBufferSize, transferQueue);
    deviceVK->copyBuffer(indexStaging.buffer, indices.buffer, indexBufferSize, transferQueue);

    scene->Destroy();
    manager->Destroy();

    return true;
}

void Model::checkMaterials()
{
    for (int i = 0; i < materialLookup.size(); ++i)
    {
        auto& material = materialLookup[i];
        if (material.textureMap.find("BaseColor") == material.textureMap.end())
        {
            throw std::runtime_error("No base color texture");
        }

        if (material.textureMap.find("Normal") == material.textureMap.end())
        {
            throw std::runtime_error("No normal texture");
        }

        if (material.textureMap.find("Metallic") == material.textureMap.end())
        {
            material.textureMap["Metallic"] = -1;
        }

        if (material.textureMap.find("Roughness") == material.textureMap.end())
        {
            material.textureMap["Roughness"] = -1;
        }
    }
}

void Model::loadMaterials(FbxNode* pNode, const vk::raii::Queue& transferQueue)
{
    if (!pNode) return;
    int matCount = pNode->GetMaterialCount();
    for (size_t mi = 0; mi < matCount; ++mi)
    {
        FbxSurfaceMaterial* mat = pNode->GetMaterial(mi);
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
                    texture.index = static_cast<uint32_t>(textureLookup.size() - 1);

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

void Model::loadNodeRecursively(FbxNode* fbxNode, Node* parent)
{
    const auto* pNodeAttribute = fbxNode->GetNodeAttribute();

    if (pNodeAttribute && pNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
    {
        auto& node = nodeLookup.emplace_back();
        node.name = fbxNode->GetName();
        node.index = nodeLookup.size() - 1;
        node.parentIndex = parent ? parent->index : -1;
        if (parent)
            parent->childIndices.push_back(node.index);
        node.translation = FBXToGLMType(fbxNode->LclTranslation.Get());
        node.rotation = FBXToGLMType(fbxNode->LclRotation.Get());
        node.scale = FBXToGLMType(fbxNode->LclScaling.Get());
        node.matrix = FBXToGLMType(fbxNode->EvaluateLocalTransform());

        node.mesh = std::make_unique<Mesh>(deviceVK, node.matrix);

        FbxMesh* fbxMesh = fbxNode->GetMesh();
        int triangleCount = fbxMesh->GetPolygonCount();
        
        std::vector<int> triangleSmGroupLookup(triangleCount, -1);
        GetTriangleSmGroupLookup(fbxMesh, triangleCount, triangleSmGroupLookup);

        std::vector<int> triangleMaterialLookup(triangleCount, -1);
        GetTriangleMaterialLookup(fbxMesh, triangleCount, triangleMaterialLookup);

        FbxVector4* controlPoints = fbxMesh->GetControlPoints();
        const int polygonCount = fbxMesh->GetPolygonCount();
        const int vertexCount = fbxMesh->GetControlPointsCount();

        std::vector<glm::vec3> groupPositions;
        groupPositions.resize(polygonCount * 3);

        std::vector<uint32_t> polygonSizes(polygonCount, 0);
        int polygonIndexCount = 0;
        for (size_t p = 0; p < polygonCount; ++p)
        {
            // Expect polySize = 3
            polygonSizes[p] = 3;
            for (size_t v = 0; v < 3; ++v)
            {
                // Position
                const int controlPointIndex = fbxMesh->GetPolygonVertex(p, v);
                FbxVector4 cp = controlPoints[controlPointIndex];
                groupPositions[polygonIndexCount + v] = glm::vec3(static_cast<float>(cp[0]), static_cast<float>(cp[1]), static_cast<float>(cp[2]));
            }
            polygonIndexCount += 3;
        }

        auto* const mainLayer = fbxMesh->GetLayer(0);
        const int* indices = fbxMesh->GetPolygonVertices();

        // Normal
        auto* fbxNorms = mainLayer->GetNormals();
        std::vector<glm::vec3> groupNorms(polygonIndexCount, glm::zero<glm::vec3>());
        GetFBXAttributeValue(fbxNorms, groupNorms, indices, polygonIndexCount, polygonSizes, polygonCount, vertexCount, glm::zero<glm::vec3>());

        // Tangent
        // TBN = [T, cross(N, T)*sign, N]
        std::vector<glm::vec4> groupTangents;
        groupTangents.resize(polygonIndexCount, glm::zero<glm::vec4>());

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
                auto binormal = glm::cross(groupNorms[i], tangent);
                float sign = glm::dot(binormal, binormals[i]);

                groupTangents[i] = glm::vec4(tangent, sign > 0 ? 1.f : -1.f);
            }
        }

        // UV
        std::vector<glm::vec2> groupUVs[MAX_UV_SETS];
        int uvsetIndex = 0;
        for (size_t i = 0; i < fbxMesh->GetLayerCount(); i++)
        {
            FbxLayer* fbxLayer = fbxMesh->GetLayer(i);
            if (!fbxLayer)
                continue;
            FbxLayerElementUV* fbxUVs = fbxLayer->GetUVs();
            if (!fbxUVs)
                continue;

            groupUVs[uvsetIndex].resize(polygonIndexCount);
            GetFBXAttributeValue(fbxUVs, groupUVs[uvsetIndex], indices, polygonIndexCount, polygonSizes, polygonCount, vertexCount, glm::zero<glm::vec2>());
            for (size_t j = 0; j < polygonCount; ++j)
            {
                if (flipV)
                {
                    groupUVs[uvsetIndex][j].y = 1.0f - groupUVs[uvsetIndex][j].y;
                }
            }

            uvsetIndex++;
            if (uvsetIndex == MAX_UV_SETS)
                break;
        }

        // Vertex Color
        std::vector<glm::vec4> groupVertexColors(polygonIndexCount, glm::zero<glm::vec4>());
        auto* fbxVertexColors = mainLayer->GetVertexColors();

        if (fbxVertexColors)
        {
            GetFBXAttributeValue(fbxVertexColors, groupVertexColors, indices, polygonIndexCount, polygonSizes, polygonCount, vertexCount, glm::zero<glm::vec4>());
        }

        // Divide the vertices by material
        int materialCount = fbxNode->GetMaterialCount();
        for (size_t mi = 0; (mi == 0 || mi < materialCount); ++mi)
        {
            std::vector<Vertex> vertices;
            std::vector<int> vertexSmGroupLookup;
            std::vector<uint32_t> indices;
            for (size_t i = 0; i < triangleCount; ++i)
            {
                if (triangleMaterialLookup[i] == mi)
                {
                    size_t vertexStartIndex = i * 3;
                    for (size_t j = 0; j < 3; ++j)
                    {
                        size_t vertexIndex = vertexStartIndex + j;
                        Vertex v{
                            .pos = groupPositions[vertexIndex],
                            .normal = groupNorms[vertexIndex],
                            .tangent = groupTangents[vertexIndex],
                            .color = groupVertexColors[vertexIndex]
                        };
                        for (size_t setIndex = 0; setIndex < MAX_UV_SETS; ++setIndex)
                        {
                            v.uvs[setIndex] = groupUVs[setIndex].size() > vertexIndex ? groupUVs[setIndex][vertexIndex] : glm::zero<glm::vec2>();
                        }
                        
                        for (size_t k = 0; k < vertices.size(); ++k)
                        {
                            if (vertices[k].pos == v.pos)
                            {
                                if (vertexSmGroupLookup[k] == triangleSmGroupLookup[i])
                                {
                                    vertices[k].normal += v.normal;
                                    assert(vertices[k].tangent[3] * v.tangent[3] > 0 && "the w component of the tangent is incorrect.");
                                    vertices[k].tangent += v.tangent;

                                    v.normal = vertices[k].normal;
                                    v.tangent = vertices[k].tangent;
                                }
                            }
                        }

                        // Check if there are any identical vertices
                        size_t k = 0;
                        for (k = 0; k < vertices.size(); ++k)
                        {
                            if (vertices[k].pos == v.pos)
                            {
                                if (vertexSmGroupLookup[k] == triangleSmGroupLookup[i])
                                {
                                    int uvSet = 0;
                                    for (uvSet = 0; uvSet < MAX_UV_SETS; uvSet++)
                                    {
                                        if (vertices[k].uvs[uvSet] == v.uvs[uvSet])
                                        {
                                            continue;
                                        }
                                        else
                                        {
                                            break;
                                        }
                                    }
                                    if (uvSet == MAX_UV_SETS)
                                    {
                                        break;
                                    }
                                }
                            }
                        }

                        if (k == vertices.size())
                        {
                            vertices.push_back(v);
                            vertexSmGroupLookup.push_back(triangleSmGroupLookup[i]);
                        }

                        indices.push_back(k);
                    }
                }
            }

            Primitive primitive;
            size_t oldSize = vertexLookup.size();
            primitive.firstVertex = oldSize;
            primitive.vertexCount = vertices.size();
            vertexLookup.insert(vertexLookup.end(), vertices.begin(), vertices.end());
            
            std::ranges::for_each(indices, [oldSize](uint32_t& x) { x += oldSize; });
            primitive.firstIndex = indexLookup.size();
            indexLookup.insert(indexLookup.end(), indices.begin(), indices.end());
            primitive.indexCount = indices.size();
            primitive.materialIndex = mi;
            node.mesh->primitives.push_back(primitive);
        }

        for (size_t i = 0; i < fbxNode->GetChildCount(); ++i)
        {
            loadNodeRecursively(fbxNode->GetChild(i), &node);
        }
    }
    else
    {
        for (size_t i = 0; i < fbxNode->GetChildCount(); ++i)
        {
            loadNodeRecursively(fbxNode->GetChild(i), parent);
        }
    }
}

void Model::GetTriangleSmGroupLookup(FbxGeometryBase* pMesh, int triangleCount, std::vector<int>& triangleSmGroupLookup)
{
    auto* pSmoothing = pMesh->GetElementSmoothing();
    if (!pSmoothing)
        return;

    bool bDirectSm = (pSmoothing->GetReferenceMode() == FbxLayerElement::eDirect);

    for (int triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
    {
        int SmIndex = bDirectSm ? triangleIndex : pSmoothing->GetIndexArray().GetAt(triangleIndex);
        int iSmoothing = pSmoothing->GetDirectArray().GetAt(SmIndex);

        triangleSmGroupLookup[triangleIndex] = iSmoothing;
    }
}

void Model::GetTriangleMaterialLookup(FbxGeometryBase* pMesh, int triangleCount, std::vector<int>& triangleMaterialLookup)
{
    auto* pMaterial = pMesh->GetElementMaterial();
    if (!pMaterial)
        return;

    for (int triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex)
    {
        int materialIndex = pMaterial->GetIndexArray().GetAt(triangleIndex);

        triangleMaterialLookup[triangleIndex] = materialIndex;
    }
}

Node* Model::nodeFromIndex(uint32_t index)
{
    if (nodeLookup.size() > index)
        return &(nodeLookup[index]);
    else
        return nullptr;
}

void Model::createEmptyTexture(const vk::raii::Queue& transferQueue)
{
    emptyTexture.deviceVK = deviceVK;
    emptyTexture.width = 1;
    emptyTexture.height = 1;
    emptyTexture.layerCount = 1;
    emptyTexture.mipLevels = 1;
    emptyTexture.index = -1;

    vk::DeviceSize imageSize = emptyTexture.width * emptyTexture.height * 4;

    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    deviceVK->CreateBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
    void* dataStaging = stagingBufferMemory.mapMemory(0, imageSize);
    memcpy(dataStaging, 0, imageSize);
    stagingBufferMemory.unmapMemory();

    deviceVK->CreateImage(emptyTexture.width, emptyTexture.height, vk::Format::eR8G8B8A8Srgb, emptyTexture.mipLevels, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, emptyTexture.image, emptyTexture.deviceMemory);
    deviceVK->transitionImageLayout(emptyTexture.image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, transferQueue);
    deviceVK->copyBufferToImage(stagingBuffer, emptyTexture.image, static_cast<uint32_t>(emptyTexture.width), static_cast<uint32_t>(emptyTexture.height), transferQueue);
    deviceVK->transitionImageLayout(emptyTexture.image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, transferQueue);

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
        .compareOp = vk::CompareOp::eNever,
        .maxLod = (float)emptyTexture.mipLevels,
        .borderColor = vk::BorderColor::eFloatOpaqueWhite,
        /*
            The unnormalizedCoordinates field specifies which coordinate system you want to use to address texels in an image.
            If this field is VK_TRUE, then you can simply use coordinates within the [0, texWidth) and [0, texHeight) range. If it is VK_FALSE, then the texels are addressed using the [0, 1) range on all axes.
            Real-world applications almost always use normalized coordinates, because then it�s possible to use textures of varying resolutions with the exact same coordinates.
        */
        .unnormalizedCoordinates = vk::False
    };
    emptyTexture.sampler = deviceVK->logicDevice.createSampler(samplerCI);

    emptyTexture.format = vk::Format::eR8G8B8A8Srgb;

    vk::ImageViewCreateInfo viewCI{
        .image = emptyTexture.image,
        .viewType = vk::ImageViewType::e2D,
        .format = vk::Format::eR8G8B8A8Srgb,
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor, .levelCount = emptyTexture.mipLevels, .layerCount = 1 }
    };
    emptyTexture.view = deviceVK->logicDevice.createImageView(viewCI);

    emptyTexture.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    emptyTexture.descriptor.sampler = emptyTexture.sampler;
    emptyTexture.descriptor.imageView = emptyTexture.view;
    emptyTexture.descriptor.imageLayout = emptyTexture.imageLayout;
}

void Model::drawNode(Node* node, vk::raii::CommandBuffer& commandBuffer, RenderFlags renderFlags, const vk::raii::PipelineLayout& pipelineLayout, uint32_t bindImageSet)
{
    if (node->mesh)
    {
        for (auto& primitive : node->mesh->primitives)
        {
            bool skip = false;
            const auto& material = materialLookup[primitive.materialIndex];
            commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, bindImageSet, *material.descriptorSet, nullptr);
            commandBuffer.drawIndexed(primitive.indexCount, 3, primitive.firstIndex, 0, 0);
        }
    }

    for (auto& childIndex : node->childIndices)
    {
        drawNode(&nodeLookup[childIndex], commandBuffer, renderFlags, pipelineLayout, bindImageSet);
    }
}

void Model::draw(vk::raii::CommandBuffer& commandBuffer, RenderFlags renderFlags, const vk::raii::PipelineLayout& pipelineLayout, uint32_t bindImageSet)
{
    commandBuffer.bindVertexBuffers(0, *vertices.buffer, { 0 });
    commandBuffer.bindIndexBuffer(*indices.buffer, 0, vk::IndexType::eUint32);

    for (auto& node : nodeLookup)
    {
        drawNode(&node, commandBuffer, renderFlags, pipelineLayout, bindImageSet);
    }
}
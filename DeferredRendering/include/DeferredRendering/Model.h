#pragma once
#include <DeferredRendering/VulkanDevice.h>

#include <vulkan/vulkan_raii.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <set>
#include <fbxsdk.h>

enum class ShaderChannel : int8_t
{
    None = -1,
    Position = 0,
    Normal,
    Tangent,
    Color,
    TexCoord0,
    TexCoord1,
    TexCoord2,
    TexCoord3,
    TexCoord4,
    TexCoord5,
    TexCoord6,
    TexCoord7,
    BlendWeights,
    BlendIndices,

    ChannelCount
};

enum class DescriptorBindingFlags { ImageBaseColor, ImageNormalMap };
enum class AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
enum class FileLoadingFlags : uint32_t
{
    None = 0x00000000,
    PreTransformVertices = 0x00000001,
    PreMultiplyVertexColors = 0x00000002,
    FlipY = 0x00000004,
    DontLoadImages = 0x00000008
};

inline FileLoadingFlags operator|(FileLoadingFlags lhs, FileLoadingFlags rhs)
{
    return static_cast<FileLoadingFlags>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline glm::vec2 FBXToGLMType(const FbxVector2& t)
{
    return glm::vec2(
        static_cast<float>(t.mData[0]),
        static_cast<float>(t.mData[1])
    );
}

inline glm::vec3 FBXToGLMType(const FbxVector4& t)
{
    return glm::vec3(
        static_cast<float>(t.mData[0]),
        static_cast<float>(t.mData[1]),
        static_cast<float>(t.mData[2])
    );
}

inline glm::vec4 FBXToGLMType(const FbxColor& t)
{
    return glm::vec4(
        static_cast<float>(t.mRed),
        static_cast<float>(t.mGreen),
        static_cast<float>(t.mBlue),
        static_cast<float>(t.mAlpha)
    );
}

template<typename T1, typename T2>
void GetFBXAttributeValue(const FbxLayerElementTemplate<T1>* element, std::vector<T2>& output, const int* indices, const int indexCount, const std::vector<uint32_t>& polygonSizes, const int polygonCount, const int vertexCount, const T2& defaultValue)
{
    output.resize(indexCount, defaultValue);

    const FbxLayerElement::EMappingMode mappingMode = element->GetMappingMode();
    if (mappingMode == FbxLayerElement::eByControlPoint)
    {
        if (element->GetDirectArray().GetCount() != vertexCount)
        {
            return;
        }

        for (int f = 0; f < indexCount; f++)
        {
            int index = (element->GetReferenceMode() == FbxGeometryElement::eDirect)
                ? indices[f]
                : element->GetIndexArray().GetAt(indices[f]);
            output[f] = FBXToGLMType(element->GetDirectArray().GetAt(index));
        }
    }
    else if (mappingMode == FbxLayerElement::eByPolygonVertex)
    {
        for (int f = 0; f < indexCount; ++f)
        {
            int index = (element->GetReferenceMode() == FbxGeometryElement::eDirect)
                ? indices[f]
                : element->GetIndexArray().GetAt(indices[f]);
            output[f] = FBXToGLMType(element->GetDirectArray().GetAt(index));
        }
    }
    else if (mappingMode == FbxLayerElement::eByPolygon)
    {
        int wedgeIndex = 0;
        for (int f = 0; f < polygonCount; f++)
        {
            for (uint32_t e = 0; e < polygonSizes[f]; e++, wedgeIndex++)
            {
                int index = (element->GetReferenceMode() == FbxGeometryElement::eDirect)
                    ? indices[f]
                    : element->GetIndexArray().GetAt(indices[f]);
                output[wedgeIndex] = FBXToGLMType(element->GetDirectArray().GetAt(index));
            }
        }
    }
    else if (mappingMode == FbxLayerElement::eAllSame)
    {
        int index = (element->GetReferenceMode() == FbxGeometryElement::eDirect)
            ? indices[0]
            : element->GetIndexArray().GetAt(indices[0]);
        T2 value = FBXToGLMType(element->GetDirectArray().GetAt(index));
        for (int f = 0; f < indexCount; f++)
            output[f] = value;
    }
    else
    {
        throw std::runtime_error("Unsupported wedge mapping mode type.");
    }
}


/*
    fbx node
*/
struct Texture
{
    Texture() = default;
    ~Texture();
    Texture(const Texture& rhs) = delete;
    Texture& operator=(const Texture& rhs) = delete;
    Texture(Texture&& rhs) = default;
    Texture& operator=(Texture&& rhs) = default;

    VulkanDevice* deviceVK = nullptr;
    vk::raii::Image image = nullptr;
    vk::raii::DeviceMemory deviceMemory = nullptr;
    vk::raii::ImageView view = nullptr;
    uint32_t width{}, height{};
    uint32_t mipLevels{};
    uint32_t layerCount{};
    vk::ImageLayout imageLayout{};
    uint32_t index{};
    vk::DescriptorImageInfo descriptor{};
    vk::raii::Sampler sampler = nullptr;
    vk::Format format{};
    std::string path;
    std::string name;
    void updateDescriptor();
    void loadImage(std::string_view path, VulkanDevice* device, const vk::raii::Queue& queue);
};

constexpr std::string_view EmissiveFactorString = "emissive";
inline const std::set<std::string_view> FloatParamArray = {
    EmissiveFactorString
};

struct Material
{
    VulkanDevice* deviceVK = nullptr;
    AlphaMode alphaMode = AlphaMode::ALPHAMODE_OPAQUE;
    //float alphaCutoff = 1.0f;
    //float metallicFactor = 1.0f;
    //float roughnessFactor = 1.0f;
    //glm::vec4 baseColorFactor = glm::vec4(1.0f);
    std::unordered_map<std::string_view, float> FloatParamMap =
    {
        {EmissiveFactorString, 0.f},
    };
    std::unordered_map<std::string, size_t> textureMap;
    vk::raii::DescriptorSet descriptorSet = nullptr;

    Material(VulkanDevice* device) : deviceVK(device) {};
    //void createDescriptorSet(vk::raii::DescriptorPool descriptorPool, vk::raii::DescriptorSetLayout descriptorSetLayout, DescriptorBindingFlags descriptorBindingFlags);
};

constexpr uint8_t MAX_UV_SETS = 8;

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec4 tangent;
    glm::vec4 color;
    glm::vec2 uvs[MAX_UV_SETS];
    //glm::vec4 joint0;
    //glm::vec4 weight0;

    /*
        A vertex binding describes at which rate to load data from memory throughout the vertices.
        It specifies the **number of bytes between data entries** and whether to move to the next data entry after each **vertex** or after each **instance**.
    */
    static vk::VertexInputBindingDescription getBindingDescription()
    {
        return {
            // The binding parameter specifies the index of the binding in the array of bindings.
            0,
            // The stride parameter specifies the number of bytes from one entry to the next
            sizeof(Vertex),
            /*
                The inputRate parameter can have one of the following values:
                    VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex
                    VK_VERTEX_INPUT_RATE_INSTANCE: Move to the next data entry after each instance
            */
            vk::VertexInputRate::eVertex };
    }

    static std::array<vk::VertexInputAttributeDescription, 12> getAttributeDescriptions()
    {
        /*
            The **location** parameter references the location directive of the input in the vertex shader.
            The **binding** parameter tells Vulkan from which binding the per-vertex data comes.
            The **format** parameter describes the type of data for the attribute.
                float: VK_FORMAT_R32_SFLOAT
                float2: VK_FORMAT_R32G32_SFLOAT
                float3: VK_FORMAT_R32G32B32_SFLOAT
                float4: VK_FORMAT_R32G32B32A32_SFLOAT

                int2: VK_FORMAT_R32G32_SINT, a 2-component vector of 32-bit signed integers
                uint4: VK_FORMAT_R32G32B32A32_UINT, a 4-component vector of 32-bit unsigned integers
                double: VK_FORMAT_R64_SFLOAT, a double-precision (64-bit) float
            The **offset** parameter has specified the number of bytes since the start of the per-vertex data to read from. T
        */

        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, tangent)),
            vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, color)),
            // todo: optimize uvs
            vk::VertexInputAttributeDescription(4, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uvs[0])),
            vk::VertexInputAttributeDescription(5, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uvs[1])),
            vk::VertexInputAttributeDescription(6, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uvs[2])),
            vk::VertexInputAttributeDescription(7, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uvs[3])),
            vk::VertexInputAttributeDescription(8, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uvs[4])),
            vk::VertexInputAttributeDescription(9, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uvs[5])),
            vk::VertexInputAttributeDescription(10, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uvs[6])),
            vk::VertexInputAttributeDescription(11, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uvs[7]))
        };
    }
};

struct Primitive {
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t firstVertex;
    uint32_t vertexCount;
    size_t materialIndex;

    struct Dimensions {
        glm::vec3 min = glm::vec3(FLT_MAX);
        glm::vec3 max = glm::vec3(-FLT_MAX);
        glm::vec3 size;
        glm::vec3 center;
        float radius;
    } dimensions;

    void SetDimensions(glm::vec3 min, glm::vec3 max);
    Primitive(uint32_t firstIndex, uint32_t indexCount, size_t materialIndex) : firstIndex(firstIndex), indexCount(indexCount), materialIndex(materialIndex) {};
};

struct Mesh
{
    VulkanDevice* deviceVK = nullptr;
    std::vector<Primitive> primitives;
    std::string name;

    struct UniformBuffer {
        vk::raii::Buffer buffer = nullptr;
        vk::raii::DeviceMemory memory = nullptr;
        vk::DescriptorBufferInfo descriptor;
        vk::raii::DescriptorSet descriptorSet = nullptr;
        void* mapped;
    } uniformBuffer;

    struct UniformBlock {
        glm::mat4 matrix;
        glm::mat4 jointMatrix[64]{};
        float jointcount{ 0 };
    } uniformBlock;

    Mesh(VulkanDevice* deviceVK, glm::mat4 matrix);
    ~Mesh();
};

struct Skin
{
    std::string name;
    size_t skeletonRootNodeIndex;
    std::vector<size_t> jointNodeIndices;
    std::vector<glm::mat4> inverseBindMatrices;
};

struct Node
{
    size_t index;
    size_t parentIndex;
    std::vector<size_t> childIndices;
    glm::mat4 matrix;
    std::string name;
    Mesh* mesh = nullptr;
    size_t skinIndex;
    glm::vec3 translation{};
    glm::vec3 scale{ 1.0f };
    glm::quat rotation{};
    glm::mat4 LocalMatrix();
    glm::mat4 GetMatrix();
    void Update();
    ~Node();
};

// struct Animation
// {

// };


class Model final
{
public:
    Model() noexcept = default;
    Model(const Model& rhs) = delete;
    Model& operator=(const Model& rhs) = delete;
    Model(Model&& rhs) = delete;
    Model& operator=(Model&& rhs) = delete;
    ~Model() = default;
public:
    /*
        Processing images will involve using a queue.
    */
    bool loadFromFile(std::string filename, VulkanDevice* device, const vk::raii::Queue& transferQueue, FileLoadingFlags fileLoadingFlags = FileLoadingFlags::None, float scale = 1.0f);
     void loadNodeRecursively(FbxNode* node);
    // void loadSkins();
     //bool getTextureMetaFromFile(const std::string& path, Texture& outMeta);
     void loadMaterials(FbxNode* node, const vk::raii::Queue& transferQueue);
    // void loadAnimations();
public:
    //Node* findNode(Node* parent, uint32_t index);
    //Node* nodeFromIndex(uint32_t index);
public:
    //void bindBuffers(vk::raii::CommandBuffer& commandBuffer);
    //void prepareNodeDescriptor(Node* node, vk::raii::DescriptorSetLayout& descriptorSetLayout);
    //void drawNode(Node* node, vk::raii::CommandBuffer& commandBuffer, uint32_t renderFlags = 0, const vk::raii::PipelineLayout& pipelineLayout = nullptr, uint32_t bindImageSet = 1);
    void draw(vk::raii::CommandBuffer& commandBuffer, uint32_t renderFlags = 0, const vk::raii::PipelineLayout& pipelineLayout = nullptr, uint32_t bindImageSet = 1);

    //void getNodeDimensions(Node* node, glm::vec3& min, glm::vec3& max);
    //void getSceneDimensions();
    // void updateAnimation(uint32_t index, float time);
public:
    struct Vertices
    {
        int count;
        vk::raii::Buffer buffer = nullptr;
        vk::raii::DeviceMemory memory = nullptr;
    } vertices;

    struct Indices
    {
        int count;
        vk::raii::Buffer buffer = nullptr;
        vk::raii::DeviceMemory memory = nullptr;
    } indices;

    struct Dimensions {
        glm::vec3 min = glm::vec3(FLT_MAX);
        glm::vec3 max = glm::vec3(-FLT_MAX);
        glm::vec3 size;
        glm::vec3 center;
        float radius;
    } dimensions;

    std::vector<Vertex> vertexLookup;

    std::vector<Node> nodeLookup;

    std::vector<Skin> skinLookup;

    std::vector<Texture> textureLookup;
    std::vector<Material> materialLookup;

    VulkanDevice* deviceVK = nullptr;

    // std::vector<Animation> animations;

    bool buffersBound = false;
    std::string path;

    inline static std::unordered_map<std::string, std::string> FBXPropertyToNew = {
        // Maya standard surface workflow which supports sd material->maya->fbx.
        // Arnold standard surface has the same mapping so it should also work.
        {"baseColor", "BaseColor"},
        {"normalCamera", "Normal"},
        {"transmissionColor", "Metallic"},
        {"specularColor", "Roughness"},
        {"specularRoughness", "Roughness"},
        {"emissionColor", "EmissiveColor"},
        {"metalness", "Metallic"},
        // UE import fbx workflow
        {FbxSurfaceMaterial::sDiffuse, "BaseColor"},
        {FbxSurfaceMaterial::sNormalMap, "Normal"},
        {FbxSurfaceMaterial::sBump, "Normal"},
        {FbxSurfaceMaterial::sSpecularFactor, "Roughness"},
        {FbxSurfaceMaterial::sShininess, "Metallic"},
        {FbxSurfaceMaterial::sEmissive, "EmissiveColor"},
        {FbxSurfaceMaterial::sAmbient, "AmbientColor"},
        {FbxSurfaceMaterial::sSpecular, "Specular"},
        {FbxSurfaceMaterial::sTransparentColor, "Opacity"},
        {FbxSurfaceMaterial::sTransparencyFactor, "OpacityMask"}
    };
private:
    bool flipV = true;
};
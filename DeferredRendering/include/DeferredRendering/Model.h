#pragma once
#include <DeferredRendering/VulkanDevice.h>

#include <vulkan/vulkan_raii.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <fbxsdk.h>

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
    vk::DescriptorImageInfo descriptor;
    vk::raii::Sampler sampler = nullptr;
    std::string format;
    void updateDescriptor();
    void loadImage(std::string_view path, VulkanDevice* device, const vk::raii::Queue& queue);
};

struct Material
{
    VulkanDevice* deviceVK = nullptr;
    AlphaMode alphaMode = AlphaMode::ALPHAMODE_OPAQUE;
    float alphaCutoff = 1.0f;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    std::unordered_map<std::string, size_t> textureMap;
    vk::raii::DescriptorSet descriptorSet = nullptr;

    Material(VulkanDevice* device) : deviceVK(device) {};
    //void createDescriptorSet(vk::raii::DescriptorPool descriptorPool, vk::raii::DescriptorSetLayout descriptorSetLayout, DescriptorBindingFlags descriptorBindingFlags);
};

constexpr uint8_t MAX_UV_SETS = 8;

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv[MAX_UV_SETS];
    glm::vec4 color;
    //glm::vec4 joint0;
    //glm::vec4 weight0;
    glm::vec4 tangent;
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

    void setDimensions(glm::vec3 min, glm::vec3 max);
    Primitive(uint32_t firstIndex, uint32_t indexCount, size_t materialIndex) : firstIndex(firstIndex), indexCount(indexCount), materialIndex(materialIndex) {};
};

struct Mesh
{
    VulkanDevice* deviceVK = nullptr;
    std::vector<Primitive> primitives;
    std::string name;

    struct UniformBuffer {
        vk::raii::Buffer buffer;
        vk::raii::DeviceMemory memory;
        vk::DescriptorBufferInfo descriptor;
        vk::raii::DescriptorSet descriptorSet = VK_NULL_HANDLE;
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
    Mesh mesh;
    size_t skinIndex;
    glm::vec3 translation{};
    glm::vec3 scale{ 1.0f };
    glm::quat rotation{};
    glm::mat4 localMatrix();
    glm::mat4 getMatrix();
    void update();
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
    //void draw(vk::raii::CommandBuffer& commandBuffer, uint32_t renderFlags = 0, const vk::raii::PipelineLayout& pipelineLayout = nullptr, uint32_t bindImageSet = 1);

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
};
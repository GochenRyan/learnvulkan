#pragma once
#include <vulkan/vulkan_raii.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>

/*
    fbx node
*/
struct Texture
{
    vk::raii::Device* device = nullptr;
    vk::raii::Image image = nullptr;
    vk::raii::ImageView view = nullptr;
    uint32_t width, height;
    uint32_t mipLevels;
    uint32_t layerCount;
    vk::DescriptorImageInfo descriptor;
    vk::raii::Sampler sampler = nullptr;
    uint32_t index;
    void updateDescriptor();
    void loadImage(std::string_view path, vk::raii::Device* device, const vk::raii::Queue& queue);
};

enum class AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };

struct Material
{
    vk::raii::Device* device = nullptr;
    AlphaMode alphaMode = AlphaMode::ALPHAMODE_OPAQUE;
    
};

struct Skin
{

};

struct Node
{

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
    void loadFromFile(std::string filename, vk::raii::Device& device, uint32_t nodeIndex, const vk::raii::Queue& queue, uint32_t fileLoadingFlags, float scale);
    void loadNode();
    void loadSkins();
    void loadImages();
    void loadMaterials();
    // void loadAnimations();
public:
    Node* findNode(Node* parent, uint32_t index);
    Node* nodeFromIndex(uint32_t index);
public:
    void bindBuffers(vk::raii::CommandBuffer& commandBuffer);
    void prepareNodeDescriptor(Node* node, vk::raii::DescriptorSetLayout& descriptorSetLayout);
    void drawNode(Node* node, vk::raii::CommandBuffer& commandBuffer, uint32_t renderFlags = 0, const vk::raii::PipelineLayout& pipelineLayout = nullptr, uint32_t bindImageSet = 1);
    void draw(vk::raii::CommandBuffer& commandBuffer, uint32_t renderFlags = 0, const vk::raii::PipelineLayout& pipelineLayout = nullptr, uint32_t bindImageSet = 1);

    void getNodeDimensions(Node* node, glm::vec3& min, glm::vec3& max);
    void getSceneDimensions();
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

    std::vector<Node*> nodes;
    std::vector<Node*> linearNodes;

    std::vector<Skin*> skins;

    std::vector<Texture> textures;
    std::vector<Material> materials;

    vk::raii::Device* device = nullptr;

    // std::vector<Animation> animations;

    bool buffersBound = false;
    std::string path;
};
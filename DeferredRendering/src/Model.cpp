#include <DeferredRendering/Model.h>

void Texture::loadImage(std::string_view path, vk::raii::Device *device, const vk::raii::Queue &queue)
{
}

void Texture::updateDescriptor()
{
}

void Model::loadFromFile(std::string filename, vk::raii::Device &device, uint32_t nodeIndex, const vk::raii::Queue &queue, uint32_t fileLoadingFlags, float scale)
{
}
#include <DeferredRendering/VulkanApp.h>
#include <chrono>
#include <format>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <fbxsdk.h>

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void*) {
    if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
        std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
    }

    return vk::False;
}

/*
    Choosing the right settings for the swap chain:
        - Surface format (color depth)
        - Presentation mode (conditions for "swapping" images to the screen)
        - Swap extent (resolution of images in swapchain)
*/

static vk::Format chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
{
    const auto formatIt = std::ranges::find_if(availableFormats,
        [](const auto& format) {
            return format.format == vk::Format::eB8G8R8A8Srgb &&
                format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
    return formatIt != availableFormats.end() ? formatIt->format : availableFormats[0].format;
}

static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
    /*
        VK_PRESENT_MODE_MAILBOX_KHR is a very nice trade-off if energy usage is not a concern.
        It allows us to avoid tearing while still maintaining fairly low latency by rendering new images
        that are as up to date as possible right until the vertical blank. On **mobile devices**,
        where **energy usage** is more important, you will probably want to use VK_PRESENT_MODE_FIFO_KHR instead.
    */
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

static void framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<VulkanApp*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

vk::Extent2D VulkanApp::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
    /*
        At this point, the device has already specified the optimal switching chain resolution 
        (for example, the recommended resolution of the screen), and no further adjustment is required
    */
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);  // Obtain the physical pixel size of the window (in pixels)

    /*
        Ensure that the resolution conforms to the minimum and maximum pixel range 
        supported by the device to avoid errors caused by exceeding the hardware limit
    */
    return {
        std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

void VulkanApp::createSwapChain()
{
    auto surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    swapChainImageFormat = chooseSwapSurfaceFormat(physicalDevice.getSurfaceFormatsKHR(surface));
    swapChainExtent = chooseSwapExtent(surfaceCapabilities);
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    minImageCount = (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount) ? surfaceCapabilities.maxImageCount : minImageCount;
    vk::SwapchainCreateInfoKHR SwapchainCreateInfo{
        .surface = surface,
        .minImageCount = minImageCount,
        .imageFormat = swapChainImageFormat,
        .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
        .imageExtent = swapChainExtent,

        /*
            The "Layer" (also known as "Array Layer") of an Image refers to different layers within an image array, 
            used to represent multiple independent images (such as the six faces of a cube map or multiple elements of a texture array).
        */

        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = chooseSwapPresentMode(physicalDevice.getSurfacePresentModesKHR(surface)),
        .clipped = true
    };

    swapChain = vk::raii::SwapchainKHR(device, SwapchainCreateInfo);
    swapChainImages = swapChain.getImages();
}

void VulkanApp::createImageViews()
{
    vk::ImageViewCreateInfo imageViewCreateInfo{ .viewType = vk::ImageViewType::e2D, .format = swapChainImageFormat,
          .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
    for (auto image : swapChainImages)
    {
        imageViewCreateInfo.image = image;
        swapChainImageViews.emplace_back(device, imageViewCreateInfo);
    }
}

void VulkanApp::Run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void VulkanApp::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void VulkanApp::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicPipeline();
    createCommandPool();
    createDepthResources();
    createFramebuffers();
    createTextureImage();
    createTextureImageView();
    createTextureSampler();
    loadModel();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void VulkanApp::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        drawFrame();
    }

    device.waitIdle();
}

void VulkanApp::cleanup()
{
    glfwDestroyWindow(window);

    glfwTerminate();
}

void VulkanApp::createInstance()
{
    if (enableValidationLayers && !checkValidationLayerSupport())
    {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    constexpr vk::ApplicationInfo appInfo{ .pApplicationName = "Hello Triangle",
                    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                    .pEngineName = "No Engine",
                    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                    .apiVersion = vk::ApiVersion14 };

    // Get the required layers
    std::vector<char const*> requiredLayers;
    if (enableValidationLayers) {
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }

    // Check if the required layers are supported by the Vulkan implementation.
    auto layerProperties = context.enumerateInstanceLayerProperties();
    for (auto const& requiredLayer : requiredLayers)
    {
        
        if (std::ranges::none_of(layerProperties,
                                    [requiredLayer](auto const& layerProperty)
                                    { return strcmp(layerProperty.layerName, requiredLayer) == 0; }))
        {
            throw std::runtime_error("Required layer not supported: " + std::string(requiredLayer));
        }
    }

    // Get the required extensions.
    auto requiredExtensions = getRequiredExtensions();

    // Check if the required extensions are supported by the Vulkan implementation.
    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    for (auto const& requiredExtension : requiredExtensions)
    {
        if (std::ranges::none_of(extensionProperties,
            [requiredExtension](auto const& extensionProperty)
            { return strcmp(extensionProperty.extensionName, requiredExtension) == 0; }))
        {
            throw std::runtime_error("Required extension not supported: " + std::string(requiredExtension));
        }
    }
    
    vk::InstanceCreateInfo createInfo{
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
            .ppEnabledLayerNames = requiredLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data() };
    instance = vk::raii::Instance(context, createInfo);
}

void VulkanApp::setupDebugMessenger() {
    if (!enableValidationLayers) return;

    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
    vk::DebugUtilsMessageTypeFlagsEXT    messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT{
        .messageSeverity = severityFlags,
        .messageType = messageTypeFlags,
        .pfnUserCallback = &debugCallback
    };
    debugMessenger = instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
}


std::vector<const char *> VulkanApp::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers) {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    return extensions;
}

bool VulkanApp::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers)
        {
            if (strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if (!layerFound)
            return false;
    }

    return true;
}

/// <summary>
/// pick a physical device which satisfy requirements
/// </summary>
void VulkanApp::pickPhysicalDevice()
{
    std::vector<vk::raii::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
    const auto                            devIter = std::ranges::find_if(
        devices,
        [&](auto const& device)
        {
            // Check if the device supports the Vulkan 1.3 API version
            bool supportsVulkan1_3 = device.getProperties().apiVersion >= VK_API_VERSION_1_3;

            // Check if any of the queue families support graphics operations
            auto queueFamilies = device.getQueueFamilyProperties();
            bool supportsGraphics =
                std::ranges::any_of(queueFamilies, [](auto const& qfp) { return !!(qfp.queueFlags & vk::QueueFlagBits::eGraphics); });

            // Check if all required device extensions are available
            auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();
            bool supportsAllRequiredExtensions =
                std::ranges::all_of(requiredDeviceExtension,
                    [&availableDeviceExtensions](auto const& requiredDeviceExtension)
                    {
                        return std::ranges::any_of(availableDeviceExtensions,
                            [requiredDeviceExtension](auto const& availableDeviceExtension)
                            { return strcmp(availableDeviceExtension.extensionName, requiredDeviceExtension) == 0; });
                    });

            auto features = device.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
            bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy && 
                                            //features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                            features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

            return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
        });
    if (devIter != devices.end())
    {
        physicalDevice = *devIter;
    }
    else
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void VulkanApp::createLogicalDevice()
{
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    // get the first index into queueFamilyProperties which supports both graphics and present
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
    {
        /*
            Any queue family with VK_QUEUE_GRAPHICS_BIT or VK_QUEUE_COMPUTE_BIT capabilities already implicitly support VK_QUEUE_TRANSFER_BIT operations.
        */
        if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
            physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface))
        {
            // found a queue family that supports both graphics and present
            queueIndex = qfpIndex;
            break;
        }
    }
    if (queueIndex == ~0)
    {
        throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
    }

    // query for Vulkan 1.3 features
    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
        {.features = { .samplerAnisotropy = true } },                                                     // vk::PhysicalDeviceFeatures2
        {.synchronization2 = true, .dynamicRendering = false },  // vk::PhysicalDeviceVulkan13Features
        {.extendedDynamicState = true }                         // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
    };

    // create a Device
    float                     queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{ .queueFamilyIndex = queueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };
    vk::DeviceCreateInfo      deviceCreateInfo{ .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
                                                .queueCreateInfoCount = 1,
                                                .pQueueCreateInfos = &deviceQueueCreateInfo,
                                                .enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size()),
                                                .ppEnabledExtensionNames = requiredDeviceExtension.data() };

    device = vk::raii::Device(physicalDevice, deviceCreateInfo);
    queue = vk::raii::Queue(device, queueIndex, 0);
}

void VulkanApp::createSurface()
{
    /*
        how to use platform-specific extension to create a surface on Windows?
            #define VK_USE_PLATFORM_WIN32_KHR
            #define GLFW_INCLUDE_VULKAN
            #include <GLFW/glfw3.h>
            #define GLFW_EXPOSE_NATIVE_WIN32
            #include <GLFW/glfw3native.h>
            ...
            VkWin32SurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            createInfo.hwnd = glfwGetWin32Window(window);
            createInfo.hinstance = GetModuleHandle(nullptr);
            if (vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) != VK_SUCCESS) {
                throw std::runtime_error("failed to create window surface!");
            }
    */

    VkSurfaceKHR _surface;
    if (glfwCreateWindowSurface(*instance, window, nullptr, &_surface) != 0)
    {
        throw std::runtime_error("failed to create window surface!");
    }
    surface = vk::raii::SurfaceKHR(instance, _surface);
}

void VulkanApp::createGraphicPipeline()
{
    auto shaderCode = readFile(ASSETS_SRC_DIR "/Shader/DeferredRendering/deferred.spv");
    vk::raii::ShaderModule shaderModule = createShaderModule(shaderCode);

    /*
        pSpecializationInfo:
            allows you to specify values for shader constants. You can use a single shader module 
            where its behavior can be configured in pipeline creation by specifying different values for the constants used in it. 
            This is more efficient than configuring the shader using variables at render time, 
            because the compiler can do optimizations like eliminating if statements that depend on these values. 
    */
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = shaderModule,
        .pName = "vertMain"
    };

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = shaderModule,
        .pName = "fragMain"
    };

    std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = { vertShaderStageInfo , fragShaderStageInfo };
    /*
        The former is specified in the topology member and can have values like:
            VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST: line from every two vertices without reuse
            VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: the end vertex of every line is used as start vertex for the next line
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: triangle from every three vertices without reuse
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: the second and third vertex of every triangle is used as first two vertices of the next triangle
    */
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList,
        .primitiveRestartEnable = vk::False
    };

    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    vk::PipelineViewportStateCreateInfo viewportStateInfo{ .viewportCount = 1, .scissorCount = 1 };

    vk::PipelineRasterizationStateCreateInfo rasterizerCI{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eBack,
        /*
            Nothing is visible because of the Y-flip we did in the projection matrix, 
            the vertices are now being drawn in counter-clockwise order instead of clockwise order. 
            This causes backface culling to kick in and prevents any geometry from being drawn.
            The determination of face orientation occurs during the **rasterization stage**
        */
        .frontFace = vk::FrontFace::eCounterClockwise,
        .depthBiasEnable = vk::False,
        .depthBiasSlopeFactor = 1.0f,
        .lineWidth = 1.0f
    };

    vk::PipelineMultisampleStateCreateInfo multisamplingInfo{
        .rasterizationSamples = vk::SampleCountFlagBits::e1, 
        .sampleShadingEnable = vk::False 
    };

    vk::PipelineDepthStencilStateCreateInfo depthStencil{
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::True,
        .depthCompareOp = vk::CompareOp::eLess,
        .depthBoundsTestEnable = vk::False,
        .stencilTestEnable = vk::False
    };

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };
    vk::PipelineColorBlendStateCreateInfo colorBlendingInfo{ 
        .logicOpEnable = vk::False, 
        .logicOp = vk::LogicOp::eCopy, 
        .attachmentCount = 1, 
        .pAttachments = &colorBlendAttachment 
    };

    /*
        Uniform values need to be specified during pipeline creation by creating a VkPipelineLayout object. 
        Even though we won't be using them now, we are still required to create an empty pipeline layout.
    */
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ 
        .setLayoutCount = 1, 
        .pSetLayouts = &*descriptorSetLayout,
        .pushConstantRangeCount = 0 
    };
    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

    vk::PipelineVertexInputStateCreateInfo emptyInputState{};

    vk::GraphicsPipelineCreateInfo pipelineCI{
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &emptyInputState,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportStateInfo,
        .pRasterizationState = &rasterizerCI,
        .pMultisampleState = &multisamplingInfo,
        .pDepthStencilState = &depthStencil,
        .pColorBlendState = &colorBlendingInfo,
        .pDynamicState = &dynamicStateInfo,
        .layout = *pipelineLayout,
        .renderPass = *renderPass,
        /*
             Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline. 
             The idea of pipeline derivatives is that it is less expensive to set up pipelines 
             when they have much functionality in common with an existing pipeline and switching between pipelines from the same parent can also be done quicker. 
             You can either specify the handle of an existing pipeline with basePipelineHandle or reference another pipeline that is about to be created by index with basePipelineIndex.
        */
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    // The vertex coordinates of the fullscreen triangle are generated in a clockwise sequence
    rasterizerCI.cullMode = vk::CullModeFlagBits::eFront;
    // Final fullscreen composition pass pipeline
    pipelines.composition = device.createGraphicsPipeline(nullptr, pipelineCI);

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    vk::PipelineVertexInputStateCreateInfo vertexInputCI{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };
    pipelineCI.pVertexInputState = &vertexInputCI;
    rasterizerCI.cullMode = vk::CullModeFlagBits::eBack;

    // Offscreen pipeline

    auto mrtShaderCode = readFile(ASSETS_SRC_DIR "/Shader/DeferredRendering/mrt.spv");
    vk::raii::ShaderModule mrtShaderModule = createShaderModule(mrtShaderCode);

    vk::PipelineShaderStageCreateInfo mrtVertShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eVertex,
        .module = mrtShaderModule,
        .pName = "vertMain"
    };

    vk::PipelineShaderStageCreateInfo mrtFragShaderStageInfo{
        .stage = vk::ShaderStageFlagBits::eFragment,
        .module = mrtShaderModule,
        .pName = "fragMain"
    };

    std::array<vk::PipelineShaderStageCreateInfo, 2> mrtShaderStages = { mrtVertShaderStageInfo , mrtFragShaderStageInfo };
    pipelineCI.renderPass = offScreenFramebuffer.renderPass;

    std::array<vk::PipelineColorBlendAttachmentState, 3> blendAttachmentStates = {
        vk::PipelineColorBlendAttachmentState {.blendEnable = vk::False, .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,},
        vk::PipelineColorBlendAttachmentState {.blendEnable = vk::False, .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,},
        vk::PipelineColorBlendAttachmentState {.blendEnable = vk::False, .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,}
    };
    colorBlendingInfo.attachmentCount = static_cast<uint32_t>(blendAttachmentStates.size());
    colorBlendingInfo.pAttachments = blendAttachmentStates.data();

    pipelines.offscreen = device.createGraphicsPipeline(nullptr, pipelineCI);
}

std::vector<char> VulkanApp::readFile(std::string_view filePath)
{
    // ate: Start reading at the end of the file
    // binary: Read the file as a binary file (avoid text transformations)
    std::ifstream file(filePath.data(), std::ios::ate | std::ios::binary);
    
    if (!file.is_open())
    {
        throw std::runtime_error(std::format("failed to open file : {0}", filePath));
    }

    // The advantage of starting to read at the end of the file is that we can use the read position to determine the size of the file and allocate a buffer
    std::vector<char> buffer(file.tellg());

    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    file.close();

    return buffer;
}

vk::raii::ShaderModule VulkanApp::createShaderModule(const std::vector<char>& code) const
{
    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };

    vk::raii::ShaderModule shaderModule{ device, createInfo };

    return shaderModule;
}

void VulkanApp::createCommandPool()
{
    vk::CommandPoolCreateInfo poolInfo{
        /*
            eTransient(VK_COMMAND_POOL_CREATE_TRANSIENT_BIT) : the command buffer in the command pool is for short-term use (with a short lifecycle) and will be released or reset shortly after use.
                One-time commands (such as UI rendering per frame)
            eResetCommandBuffer(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT) : individual command buffers allocated from the command pool are allowed to be reset independently (via vkResetCommandBuffer()).
                The command buffer needs to be frequently reused (such as recording commands once per frame)
            eProtected(VK_COMMAND_POOL_CREATE_PROTECTED_BIT) : indicate that the command buffer in the command pool is a protected resource for handling sensitive data (such as encrypted content).
        */
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueIndex
    };
    commandPool = vk::raii::CommandPool(device, poolInfo);
}

void VulkanApp::createCommandBuffers()
{
    commandBuffers.clear();
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };

    commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
}

void VulkanApp::recordCommandBuffer(uint32_t imageIndex)
{
    commandBuffers[currentFrame].begin({});

    // First render pass : Offscreen pass to fill deferred attachments
    {
        std::array < vk::ClearValue, 4> clearValues{
            vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f),
            vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f),
            vk::ClearColorValue(0.0f, 0.0f, 0.0f, 0.0f),
            vk::ClearDepthStencilValue(1.0f, 0)
        };
    
        vk::RenderPassBeginInfo renderPassbeginInfo{
            .renderPass = offScreenFramebuffer.renderPass,
            .framebuffer = offScreenFramebuffer.framebuffer,
            .renderArea = {.offset = { 0, 0 }, .extent = { offScreenFramebuffer.width, offScreenFramebuffer.height } },
            .clearValueCount = static_cast<uint32_t>(clearValues.size()),
            .pClearValues = clearValues.data()
        };

        commandBuffers[currentFrame].beginRenderPass(renderPassbeginInfo, vk::SubpassContents::eInline);
        commandBuffers[currentFrame].setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(offScreenFramebuffer.width), static_cast<float>(offScreenFramebuffer.height), 0.0f, 1.0f));
        commandBuffers[currentFrame].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(offScreenFramebuffer.width, offScreenFramebuffer.height)));
        commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *pipelines.offscreen);

        // Floor
        commandBuffers[currentFrame].bindDescriptorSets(
            // Unlike vertex and index buffers, descriptor sets are not unique to graphics pipelines. Therefore, we need to specify if we want to bind descriptor sets to the graphics or compute pipeline. 
            vk::PipelineBindPoint::eGraphics,
            // The layout that the descriptors are based on. 
            pipelineLayout,
            // The index of the first descriptor set
            0,
            *descriptorSets[currentFrame].floor,
            nullptr);
        //models.floor.draw(cmdBuffer);
    }
}

void VulkanApp::transition_image_layout(uint32_t imageIndex, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask)
{
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = swapChainImages[imageIndex],
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    vk::DependencyInfo dependency_info = {
            .dependencyFlags = {},
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier
    };
    commandBuffers[currentFrame].pipelineBarrier2(dependency_info);
}

/*
    Rendering a frame:
        Wait for the previous frame to finish
        Acquire an image from the swap chain
        Record a command buffer which draws the scene onto that image
        Submit the recorded command buffer
        Present the swap chain image
*/
void VulkanApp::drawFrame()
{
    /*
        Since MAX_FRAMES_IN_FLIGHT is greater than 1, when the CPU is preparing for the next frame, the GPU is processing the previous frame, while device.waitForFences checks the fence of the current frame. 
        The fence of this frame is usually not triggered yet (because the GPU has not started processing the current frame), so the CPU will not block
    */
    while (vk::Result::eTimeout == device.waitForFences(*inFlightFences[currentFrame], vk::True, UINT16_MAX));

    auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *presentCompleteSemaphores[semaphoreIndex], nullptr);

    /*
        VK_ERROR_OUT_OF_DATE_KHR: The swap chain has become incompatible with the surface and can no longer be used for rendering. Usually happens after a window resize.
        VK_SUBOPTIMAL_KHR: The swap chain can still be used to successfully present to the surface, but the surface properties are no longer matched exactly.
    */
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized)
    {
        framebufferResized = false;
        recreateSwapChain();
        return;
    }

    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
    {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    updateUniformBuffer(currentFrame);

    device.resetFences(*inFlightFences[currentFrame]);
    commandBuffers[currentFrame].reset();
    recordCommandBuffer(imageIndex);

    /*
        Why need semaphores?
            The execution model of a GPU is highly parallel, typically involving multiple queues (such as graphics queues, computing queues, and transmission queues) and complex resource dependencies. 
            The role of semaphores is to explicitly control the dependencies of these concurrent operations, avoiding resource conflicts or invalid access.

            Cross-queue synchronization:
                Hardware limitations: 
                    Different queues of the GPU may run on **different hardware units** (such as the graphics engine and the computing engine). 
                    If two queues access the same resource (such as an image or buffer) simultaneously, 
                    it is necessary to ensure through a semaphore that the operation of the previous queue is completed before starting the operation of the next queue.
                    
                Example:
                    After the rendering queue generates an image, it is necessary to notify the rendering queue through a semaphore that the image can be submitted to the screen.
                    After the computing queue has completed data processing, it needs to notify the graphic queue through a semaphore that the results can be read.
            
            Internal synchronization of the queue:
                Even within the same queue, there may be dependencies among multiple command buffers. 
                Semaphores can ensure that the execution of subsequent command buffers is triggered only after the previous command buffer has been completed.
    */

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo{
        /*
            The first three parameters specify which semaphores to wait on before execution begins and in which stage(s) of the pipeline to wait.
        */
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*presentCompleteSemaphores[semaphoreIndex],
        .pWaitDstStageMask = &waitDestinationStageMask,

        /*
            specifies which command buffers to actually submit for execution
        */
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffers[currentFrame],

        /*
            specifies which semaphores to signal once the command buffer(s) have finished execution
        */
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*renderFinishedSemaphores[imageIndex]
    };

    queue.submit(submitInfo, 
        /*
            signaled when the command buffers finish execution
        */
        *inFlightFences[currentFrame]);

    //vk::SubpassDependency dependency{
    //    .srcSubpass = VK_SUBPASS_EXTERNAL,  // The special value VK_SUBPASS_EXTERNAL refers to the implicit subpass before or after the render pass depending on whether it is specified in srcSubpass or dstSubpass. 
    //    .dstSubpass = {},  // The index 0 refers to our subpass, which is the first and only one. The dstSubpass must always be higher than srcSubpass to prevent cycles in the dependency graph (unless one of the subpasses is **VK_SUBPASS_EXTERNAL**).
    //    .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
    //    .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
    //    .srcAccessMask = {},
    //    .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite
    //};

    const vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*renderFinishedSemaphores[semaphoreIndex],
        .swapchainCount = 1,
        .pSwapchains = &*swapChain,
        .pImageIndices = &imageIndex
    };

    // The vkQueuePresentKHR function submits the request to present an image to the swap chain. 
    // The vkQueuePresentKHR function submits the request to present an image to the swap chain. 
    try
    {
        // presentKHR will throw on eErrorOutOfDateKHR
        queue.presentKHR(presentInfoKHR);
    }
    catch (vk::OutOfDateKHRError const&)
    {
        result = vk::Result::eErrorOutOfDateKHR;
    }

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    }
    else if (result != vk::Result::eSuccess) {
        throw std::runtime_error("failed to present swap chain image!");
    }
    semaphoreIndex = (semaphoreIndex + 1) % presentCompleteSemaphores.size();
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanApp::createSyncObjects()
{
    presentCompleteSemaphores.clear();
    renderFinishedSemaphores.clear();
    inFlightFences.clear();

    for (size_t i = 0; i < swapChainImages.size(); ++i)
    {
        presentCompleteSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());  // Ensure that the image is obtained from the Swap Chain before the rendering queue can use the image
        renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo());  // Notify that the rendering of the presentation queue has been completed and images can be submitted to the screen
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        inFlightFences.emplace_back(device, vk::FenceCreateInfo { .flags = vk::FenceCreateFlagBits::eSignaled });
    }
}

void VulkanApp::recreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    device.waitIdle();

    cleanupSwapChain();
    createSwapChain();
    createImageViews();
    createDepthResources();
    createFramebuffers();
}

void VulkanApp::cleanupSwapChain()
{
    /*
        The disadvantage of this approach is that we need to stop all renderings before creating the new swap chain. 
        It is possible to create a new swap chain while drawing commands on an image from the old swap chain are still in-flight. 
        You need to pass the previous swap chain to the oldSwapchain field in the VkSwapchainCreateInfoKHR struct and destroy the old swap chain as soon as you've finished using it.
    */
    swapChainImageViews.clear();
    swapChain = nullptr;
}


/*
    Buffers in Vulkan are regions of memory used for storing arbitrary data that can be read by the graphics card. 
*/
void VulkanApp::createVertexBuffer()
{
    //vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    //createBuffer(bufferSize, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vertexBuffer, vertexBufferMemory);
    //void* data = vertexBufferMemory.mapMemory(0, bufferSize);
    ///*
    //    Unfortunately, the driver may not immediately copy the data into the buffer memory, for example, because of caching. 
    //    It is also possible that writes to the buffer are not visible in the mapped memory yet. There are two ways to deal with that problem:
    //        Use a memory heap that is host coherent, indicated with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    //        Call vkFlushMappedMemoryRanges after writing to the mapped memory, and call vkInvalidateMappedMemoryRanges before reading from the mapped memory

    //    Flushing memory ranges or using a coherent memory heap means that the driver will be aware of our writings to the buffer, but it doesn't mean that they are actually visible on the GPU yet. 
    //    The transfer of data to the GPU is an operation that happens in the background, and the specification simply tells us that it is guaranteed to be complete as of the next call to vkQueueSubmit.
    //*/
    //memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    //vertexBufferMemory.unmapMemory();

    /*
        The memory type that allows us to access it from the CPU may not be the most optimal memory type for the graphics card itself to read from. 
        The most optimal memory has the VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT flag and is usually not accessible by the CPU on dedicated graphics cards.
        =>  One staging buffer in CPU accessible memory to upload the data from the vertex array to, and the final vertex buffer in device local memory. 
            We'll then use a buffer copy command to move the data from the staging buffer to the actual vertex buffer.
    */

    /*
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT: Buffer can be used as source in a memory transfer operation.
        VK_BUFFER_USAGE_TRANSFER_DST_BIT: Buffer can be used as destination in a memory transfer operation.
    */
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
    void* dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(dataStaging, vertices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();
    /*
        The DEVICE_LOCAL_BIT flag indicates that the memory is video memory (dedicated to the GPU) and can only be accessed by the GPU. 
        This kind of memory cannot be mapped by the CPU (that is, the pointer cannot be obtained through vkMapMemory), because the address space of the video memory is invisible to the CPU.

        If the data is directly written to the buffer in the system memory through the CPU, the GPU may need to wait for the CPU to complete the writing before it can start processing the data. 
        By using the staging buffer and the GPU's transmission queue, asynchronous transmission can be achieved to avoid blocking the GPU.
    */
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal, vertexBuffer, vertexBufferMemory);
    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
}

/*
    Graphics cards can offer different types of memory to allocate from. 
    Each type of memory varies in terms of allowed operations and performance characteristics. 
    We need to combine the requirements of the buffer and our own application requirements to find the right type of memory to use.
*/
uint32_t VulkanApp::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
{
    /*
        The VkPhysicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps. 
        Memory heaps are distinct memory resources like **dedicated VRAM** and **swap space in RAM** for when VRAM runs out. 
        The different types of memory exist within these heaps.
    */
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void VulkanApp::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory)
{
    vk::BufferCreateInfo bufferInfo{
        // used to configure sparse buffer memory,
        .flags = {},
        // specifies the size of the buffer in bytes.
        .size = size,
        // indicates for which purposes the data in the buffer is going to be used. It is possible to specify multiple purposes using a bitwise or.
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive
    };
    buffer = vk::raii::Buffer(device, bufferInfo);

    /*
        The VkMemoryRequirements struct has three fields:
            size: The size of the required memory in bytes may differ from bufferInfo.size.
            alignment: The offset in bytes where the buffer begins in the allocated region of memory, depends on bufferInfo.usage and bufferInfo.flags.
            memoryTypeBits: Bit field of the memory types that are suitable for the buffer.
    */
    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

    vk::MemoryAllocateInfo memoryAllocateInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
    };

    /*
        It should be noted that in a real world application, you're not supposed to actually call vkAllocateMemory for every individual buffer. 
        The maximum number of simultaneous memory allocations is limited by the maxMemoryAllocationCount physical device limit, which may be as low as 4096 even on high end hardware like an NVIDIA GTX 1080. 
        The right way to allocate memory for a large number of objects at the same time is to create a custom allocator that splits up a single allocation among many different objects 
        by using the offset parameters that we've seen in many functions.

        You can either implement such an allocator yourself, or use the VulkanMemoryAllocator library provided by the GPUOpen initiative. 
        However, for this tutorial, it's okay to use a separate allocation for every resource, because we won't come close to hitting any of these limits for now.
    */
    bufferMemory = vk::raii::DeviceMemory(device, memoryAllocateInfo);
    buffer.bindMemory(*bufferMemory, /* the offset within the region of memory. If the offset is non-zero, then it is required to be divisible by memRequirements.alignment. */0);
}

void VulkanApp::copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size)
{
    auto commandCopyBuffer = beginSingleTimeCommands();
    commandCopyBuffer->copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
    endSingleTimeCommands(*commandCopyBuffer);
}

void VulkanApp::createIndexBuffer()
{
    vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
    void* dataStaging = stagingBufferMemory.mapMemory(0, bufferSize);
    memcpy(dataStaging, indices.data(), bufferSize);
    stagingBufferMemory.unmapMemory();

    createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal, indexBuffer, indexBufferMemory);
    copyBuffer(stagingBuffer, indexBuffer, bufferSize);
}

void VulkanApp::createUniformBuffers()
{
    for (auto& buffer : uniformBuffers)
    {
        // Offscreen
        createBuffer(sizeof(UniformDataOffscreen), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer.offscreen.uniformBuffer, buffer.offscreen.uniformBufferMemory);
        buffer.offscreen.uniformBuffersMapped = buffer.offscreen.uniformBufferMemory.mapMemory(0, sizeof(UniformDataOffscreen));
        // Composition
        createBuffer(sizeof(UniformDataComposition), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer.composition.uniformBuffer, buffer.composition.uniformBufferMemory);
        buffer.composition.uniformBuffersMapped = buffer.composition.uniformBufferMemory.mapMemory(0, sizeof(UniformDataComposition));
    }

    // Setup instanced model positions
    uniformDataOffscreen.instancePos[0] = glm::vec4(0.0f);
    uniformDataOffscreen.instancePos[1] = glm::vec4(-4.0f, 0.0, -4.0f, 0.0f);
    uniformDataOffscreen.instancePos[2] = glm::vec4(4.0f, 0.0, -4.0f, 0.0f);
}

void VulkanApp::updateUniformBuffer(uint32_t currentFrame)
{
    uniformDataOffscreen.projection = glm::perspective(glm::radians(60.0f), static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height), 0.1f, 256.0f);
    // GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix. 
    uniformDataOffscreen.projection[1][1] *= -1;
    uniformDataOffscreen.view = glm::lookAt(glm::vec3(2.15f, 0.3f, -8.75f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    uniformDataOffscreen.model = glm::mat4(1.0f);
    memcpy(uniformBuffers[currentFrame].offscreen.uniformBuffersMapped, &uniformDataOffscreen, sizeof(UniformDataOffscreen));
}

/*
    A descriptor is a way for shaders to freely access resources like buffers and images.
    We're going to set up a buffer that contains the transformation matrices and have the vertex shader access them through a descriptor. Usage of descriptors consists of three parts:
        Specify a descriptor set layout during pipeline creation
        Allocate a descriptor set from a descriptor pool
        Bind the descriptor set during rendering
*/
/*
    Different between descriptor design and vertex/index design
        Semantics and usage scenarios are different
            Vertex/Index: This belongs to the input assembly stage. The GPU requires a continuous vertex stream/index stream, and the access mode is simple (continuous reading, fixed format). Binding buffer + offset directly to IA (input assembly) is low-overhead and intuitive.
            Descriptor (Uniform/Storage/Texture/Sampler) : is the shader anywhere access to the internal resources, may be an array, random access, across the shader stages, different life cycle and align with the format requirements, access pattern is complicated.
        Indirect addressing and indexing are required
            Shaders often need to access a large number of resources (texture arrays, bindless) by index. The Descriptor provides an intermediate table (descriptor set) - the shader only sees the index/handle, and the actual location of the physical resource is pointed to by the descriptor.
            Directly "writing Pointers into the command stream" like vertex does cannot effectively support such dynamic indexing or massive resource collections.
        The lifecycle is different from the reuse strategy
            Vertex/Index buffering is usually used directly once or several times in the short term; Material maps, Samplers, uniforms and other resources will be reused for a long time after loading.
            Descriptor allows long-term unchanging objects (textures) to be updated and reused at one time instead of being rewritten each time a draw is made.
        Driver/hardware predictability and preprocessing
            The Descriptor layout can describe the expected resource structure of the shader when the pipeline is created, and the driver can pre-allocate the hardware table or perform verification/optimization.
            It is very difficult for the driver to perform such "pre-compilation" optimization each time vertex/index is bound. Predictability is at the core of the Vulkan performance model.
        Concurrency and Multithreading preparation
            Descriptor sets can be pre-built/updated in CPU multithreading, and then only low-cost bindings are performed in the rendering hot path.
            Repeatedly modifying a large number of resources during draw will hinder multi-threaded recording and efficient parallelism.
        Support advanced features (dynamic offsets/bindless/push descriptors)
            The Descriptor system allows functions such as dynamic offset (the same descriptor pointing to different segments of the large buffer) and descriptor indexing (close to bindless),
            which cannot be naturally expressed by the traditional vertex/index binding.
        Memory management and fragmentation control
            With the concepts of descriptor pool and sets, the application can control the allocation strategy, reclaim and reset, avoiding uncontrollable allocation by the driver in the hot path.
    =>
        The core of descriptor is "explicit indirect resource description" - it declares the structure and binding relationship of resources, enabling drivers and hardware to prepare, reuse and parallelize in advance,
        rather than turning resource binding into unordered and unpredictable runtime work.
*/
void VulkanApp::createDescriptorSetLayout()
{
    std::array bindings = {
        // Binding 0 : Vertex shader uniform buffer
        vk::DescriptorSetLayoutBinding {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
            .pImmutableSamplers = nullptr
        },
        // Binding 1 : Position texture target / Scene colormap
        vk::DescriptorSetLayoutBinding {
            .binding = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
            .pImmutableSamplers = nullptr
        },
        // Binding 2 : Normals texture target
        vk::DescriptorSetLayoutBinding {
            .binding = 2,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
            .pImmutableSamplers = nullptr
        },
        // Binding 3 : Albedo texture target
        vk::DescriptorSetLayoutBinding {
            .binding = 3,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
            .pImmutableSamplers = nullptr
        },
        // Binding 4 : Fragment shader uniform buffer
        vk::DescriptorSetLayoutBinding {
            .binding = 4,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eFragment,
            .pImmutableSamplers = nullptr
        }
    };
    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };
    descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}

void VulkanApp::createDescriptorPool()
{
    std::array poolSizes{
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eUniformBuffer,
            // Q: Why 8?
            .descriptorCount = MAX_FRAMES_IN_FLIGHT * 8
        },
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eCombinedImageSampler,
            // Q: Why 9?
            .descriptorCount = MAX_FRAMES_IN_FLIGHT * 9
        }
    };
    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        // Deferred composition: 1 + Offscreen (scene): 2 = 3
        .maxSets = MAX_FRAMES_IN_FLIGHT * 3,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };
    descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
}

void VulkanApp::createDescriptorSets()
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };

    // Sets per frame, just like the buffers themselves
    // Image descriptors for the offscreen color attachments
    vk::DescriptorImageInfo descriptorPosition{
        .sampler = colorSampler,
        .imageView = offScreenFramebuffer.position.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };
    vk::DescriptorImageInfo descriptorNormal{
        .sampler = colorSampler,
        .imageView = offScreenFramebuffer.normal.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };
    vk::DescriptorImageInfo descriptorAlbedo{
        .sampler = colorSampler,
        .imageView = offScreenFramebuffer.albedo.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };

    // Model
    vk::DescriptorImageInfo modelDescriptorColorMap{
        .sampler = textureSampler,
        .imageView = offScreenFramebuffer.normal.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };
    vk::DescriptorImageInfo modelDescriptorNormalMap{
        .sampler = textureSampler,
        .imageView = offScreenFramebuffer.normal.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };

    // Background
    vk::DescriptorImageInfo bgDescriptorColorMap{
        .sampler = textureSampler,
        .imageView = offScreenFramebuffer.normal.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };
    vk::DescriptorImageInfo bgDescriptorNormalMap{
        .sampler = textureSampler,
        .imageView = offScreenFramebuffer.normal.view,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorBufferInfo compositionBI{
            .buffer = uniformBuffers[i].composition.uniformBuffer,
            .offset = 0,
            .range = sizeof(UniformDataComposition)
        };
        std::vector<vk::WriteDescriptorSet> writeDescriptorSets;
        // Deferred composition
        descriptorSets[i].composition = std::move(device.allocateDescriptorSets(allocInfo)[0]);
        writeDescriptorSets = {
            // Binding 1 : Position texture target
            vk::WriteDescriptorSet{ .dstSet = descriptorSets[i].composition, .dstBinding = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &descriptorPosition},
            // Binding 2 : Normals texture target
            vk::WriteDescriptorSet{ .dstSet = descriptorSets[i].composition, .dstBinding = 2, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &descriptorNormal},
            // Binding 3 : Albedo texture target
            vk::WriteDescriptorSet{ .dstSet = descriptorSets[i].composition, .dstBinding = 3, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &descriptorAlbedo},
            // Binding 4 : Fragment shader uniform buffer
            vk::WriteDescriptorSet{ .dstSet = descriptorSets[i].composition, .dstBinding = 4, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &compositionBI}
        };
        device.updateDescriptorSets(writeDescriptorSets, {});

        // Offscreen (scene)
        
        // Model
        vk::DescriptorBufferInfo offscreenModelBI{
            .buffer = uniformBuffers[i].offscreen.uniformBuffer,
            .offset = 0,
            .range = sizeof(UniformDataOffscreen)
        };
        descriptorSets[i].model = std::move(device.allocateDescriptorSets(allocInfo)[0]);
        writeDescriptorSets = {
            // Binding 0: Vertex shader uniform buffer
            vk::WriteDescriptorSet{ .dstSet = descriptorSets[i].model, .dstBinding = 0, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &offscreenModelBI},
            // Binding 1: Color map
            vk::WriteDescriptorSet{.dstSet = descriptorSets[i].model, .dstBinding = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &modelDescriptorColorMap},
            // Binding 2: Normal map
            vk::WriteDescriptorSet{.dstSet = descriptorSets[i].model, .dstBinding = 2, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &modelDescriptorColorMap}
        };
        device.updateDescriptorSets(writeDescriptorSets, {});

        // Background
        descriptorSets[i].floor = std::move(device.allocateDescriptorSets(allocInfo)[0]);
        writeDescriptorSets = {
            // Binding 0: Vertex shader uniform buffer
            vk::WriteDescriptorSet{.dstSet = descriptorSets[i].floor, .dstBinding = 0, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &offscreenModelBI},
            // Binding 1: Color map
            vk::WriteDescriptorSet{.dstSet = descriptorSets[i].floor, .dstBinding = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &bgDescriptorColorMap},
            // Binding 2: Normal map
            vk::WriteDescriptorSet{.dstSet = descriptorSets[i].floor, .dstBinding = 2, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &bgDescriptorNormalMap}
        };
        device.updateDescriptorSets(writeDescriptorSets, {});
    }
}

// todo: Try to experiment with this by creating a setupCommandBuffer that the helper functions record commands into, and add a flushSetupCommands to execute the commands that have been recorded so far. 
// It’s best to do this after the texture mapping works to check if the texture resources are still set up correctly.
void VulkanApp::createTextureImage()
{
    int texWidth, texHeight, texChannels;
    std::string texturePath = ASSETS_SRC_DIR "/Model/kris-light-world-form-deltarune/textures/krish_light_form_text.png";
    stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels,
        // The STBI_rgb_alpha value forces the image to be loaded with an alpha channel, even if it doesn’t have one
        STBI_rgb_alpha);
    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels)
        throw std::runtime_error("failed to load texture image!");
    
    vk::raii::Buffer stagingBuffer = nullptr;
    vk::raii::DeviceMemory stagingBufferMemory = nullptr;
    createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
    void* dataStaging = stagingBufferMemory.mapMemory(0, imageSize);
    memcpy(dataStaging, pixels, imageSize);
    stagingBufferMemory.unmapMemory();

    stbi_image_free(pixels);

    createImage(texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage, textureImageMemory);

    transitionImageLayout(textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    transitionImageLayout(textureImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void VulkanApp::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Image& image, vk::raii::DeviceMemory& imageMemory)
{
    vk::ImageCreateInfo imageInfo{
        /*
            tells Vulkan with what kind of coordinate system the texels in the image are going to be addressed. It is possible to create 1D, 2D and 3D images. 
            One dimensional images can be used to store an array of data or gradient, 
            two dimensional images are mainly used for textures, 
            and three dimensional images can be used to store voxel volumes
        */
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {width, height, 
        //  The extent field specifies the dimensions of the image, basically how many texels there are on each axis. That’s why depth must be 1 instead of 0
        1},
        .mipLevels = 1,
        .arrayLayers = 1,
        // The samples flag is related to multisampling. This is only relevant for images that will be used as attachments, so stick to one sample. 
        .samples = vk::SampleCountFlagBits::e1,
        /*
            VK_IMAGE_TILING_LINEAR: Texels are laid out in row-major order like our pixels array
            VK_IMAGE_TILING_OPTIMAL: Texels are laid out in an implementation defined order for optimal access
        */
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
        // The image will only be used by one queue family: the one that supports graphics (and therefore also) transfer operations.
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = {},
        .initialLayout = vk::ImageLayout::eUndefined
    };

    image = vk::raii::Image(device, imageInfo);
    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties)
    };
    imageMemory = vk::raii::DeviceMemory(device, allocInfo);
    image.bindMemory(imageMemory, 0);
}

std::unique_ptr<vk::raii::CommandBuffer> VulkanApp::beginSingleTimeCommands()
{
    vk::CommandBufferAllocateInfo allocInfo{
            .commandPool = commandPool,
            .level = vk::CommandBufferLevel::ePrimary,
            .commandBufferCount = 1
    };
    std::unique_ptr<vk::raii::CommandBuffer> commandBuffer = std::make_unique<vk::raii::CommandBuffer>(std::move(vk::raii::CommandBuffers(device, allocInfo).front()));

    vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };
    commandBuffer->begin(beginInfo);

    return commandBuffer;
}

void VulkanApp::endSingleTimeCommands(vk::raii::CommandBuffer& commandBuffer)
{
    commandBuffer.end();
    queue.submit(vk::SubmitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffer
        }, nullptr);
    queue.waitIdle();
}

void VulkanApp::transitionImageLayout(const vk::raii::Image& image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout)
{
    auto commandBuffer = beginSingleTimeCommands();
    vk::ImageMemoryBarrier barrier{
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;
    if (oldLayout == vk::ImageLayout::eUndefined && 
        /*
            There is actually a special type of image layout that supports all operations, VK_IMAGE_LAYOUT_GENERAL. 
            The problem with it, of course, is that it doesn’t necessarily offer the best performance for any operation. 
            It is required for some special cases, like using an image as both input and output, or for reading an image after it has left the preinitialized layout.
        */
        newLayout == vk::ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }
    commandBuffer->pipelineBarrier(
        /*
            https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/chap7.html#synchronization-access-types-supported
        */
        // The first parameter after the command buffer specifies in which pipeline stage the operations occur that should happen before the barrier.
        sourceStage, 
        // The second parameter specifies the pipeline stage in which operations will wait on the barrier. 
        destinationStage, {}, {}, nullptr, barrier);
    endSingleTimeCommands(*commandBuffer);
}

void VulkanApp::copyBufferToImage(const vk::raii::Buffer& buffer, vk::raii::Image& image, uint32_t width, uint32_t height)
{
    auto commandBuffer = beginSingleTimeCommands();
    vk::BufferImageCopy region{
        .bufferOffset = 0,

        /*
            The bufferRowLength and bufferImageHeight fields specify how the pixels are laid out in memory. 
            For example, you could have some padding bytes between rows of the image.
        */
        .bufferRowLength = 0,
        .bufferImageHeight = 0,

        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .imageOffset = {
            .x = 0,
            .y = 0,
            .z = 0
        },
        .imageExtent = {
            .width = width,
            .height = height,
            .depth = 1
        }
    };
    commandBuffer->copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, {region});
    endSingleTimeCommands(*commandBuffer);
}

void VulkanApp::createTextureImageView()
{
    textureImageView = createImageView(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
}

vk::raii::ImageView VulkanApp::createImageView(vk::raii::Image& image, vk::Format format, vk::ImageAspectFlags aspectFlags)
{
    /*
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        Each component (r, g, b, a) in the components field specifies the mapping method of the image color channel.
        VK_COMPONENT_SWIZZLE_IDENTITY indicates no swapping, that is, the red channel remains red, the green channel remains green, and so on.
        Vulkan allows the order of image channels to be adjusted through component swapping (Swizzle) without modifying the image data itself. This is very useful in the following scenarios:
            Format mismatch: When the image format does not match the channel order expected by the shader (for example, the image is stored as BGR, but the shader expects RGB).
            Monochrome channel: Map multiple channels to the same value (for example, set the Alpha channel as the red channel).
            Simplify data processing: Avoid preprocessing image data on the CPU side.
    */

    vk::ImageViewCreateInfo viewInfo{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    return vk::raii::ImageView(device, viewInfo);
}

void VulkanApp::createTextureSampler()
{
    vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
    vk::SamplerCreateInfo samplerInfo{
        // Magnification concerns the oversampling problem describes above, and minification concerns undersampling. 
        .magFilter = vk::Filter::eLinear,
        .minFilter = vk::Filter::eLinear,

        .mipmapMode = vk::SamplerMipmapMode::eLinear,

        /*
            VK_SAMPLER_ADDRESS_MODE_REPEAT: Repeat the texture when going beyond the image dimensions.
            VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: Like repeat, but inverts the coordinates to mirror the image when going beyond the dimensions.
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: Take the color of the edge closest to the coordinate beyond the image dimensions.
            VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: Like clamp to edge, but instead uses the edge opposite to the closest edge.
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: Return a solid color when sampling beyond the dimensions of the image.
        */
        .addressModeU = vk::SamplerAddressMode::eRepeat,
        .addressModeV = vk::SamplerAddressMode::eRepeat,
        .addressModeW = vk::SamplerAddressMode::eRepeat,

        .mipLodBias = 0,
        .anisotropyEnable = vk::True,
        .maxAnisotropy = properties.limits.maxSamplerAnisotropy,

        /*
            If a comparison function is enabled, then texels will first be compared to a value, and the result of that comparison is used in filtering operations. 
            This is mainly used for percentage-closer filtering on shadow maps.
        */
        .compareEnable = vk::False,
        .compareOp = vk::CompareOp::eAlways,
        /*
            The borderColor field specifies which color is returned when sampling beyond the image with clamp to border addressing mode. 
            It is possible to return black, white or transparent in either float or int formats. You cannot specify an arbitrary color.
        */
        .borderColor = vk::BorderColor::eIntOpaqueBlack,
        /*
            The unnormalizedCoordinates field specifies which coordinate system you want to use to address texels in an image. 
            If this field is VK_TRUE, then you can simply use coordinates within the [0, texWidth) and [0, texHeight) range. If it is VK_FALSE, then the texels are addressed using the [0, 1) range on all axes. 
            Real-world applications almost always use normalized coordinates, because then it’s possible to use textures of varying resolutions with the exact same coordinates.
        */
        .unnormalizedCoordinates = vk::False
    };
    textureSampler = vk::raii::Sampler(device, samplerInfo);
}

void VulkanApp::createDepthResources()
{
    vk::Format depthFormat = findDepthFormat();

    createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
}

vk::Format VulkanApp::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    for (const auto format : candidates) {
        vk::FormatProperties props = physicalDevice.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

vk::Format VulkanApp::findDepthFormat()
{
    return findSupportedFormat(
        { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

bool VulkanApp::hasStencilComponent(vk::Format format)
{
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

bool VulkanApp::loadModel()
{
    bool flipV = true;

    std::string modelPath = ASSETS_SRC_DIR "/Model/kris-light-world-form-deltarune/source/kris.fbx";

    // Create manager & iosettings
    FbxManager* manager = FbxManager::Create();
    if (!manager) return false;
    FbxIOSettings* ios = FbxIOSettings::Create(manager, IOSROOT);
    manager->SetIOSettings(ios);

    // Create an importer
    FbxImporter* importer = FbxImporter::Create(manager, "");
    bool importOK = importer->Initialize(modelPath.c_str(), -1, manager->GetIOSettings());
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

    std::unordered_map<Vertex, uint32_t, VertexHash, VertexEqual> vtxToIndex;
    vtxToIndex.reserve(4096);

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
        FbxMesh* mesh = node->GetMesh();
        if (!mesh) continue;

        // Control points = positions
        FbxVector4* controlPoints = mesh->GetControlPoints();
        const int polygonCount = mesh->GetPolygonCount();

        // UV layer (Only process Layer 0)
        FbxGeometryElementUV* uvElement = nullptr;
        if (mesh->GetElementUVCount() > 0) {
            uvElement = mesh->GetElementUV(0);
        }

        // Vertex color layer (Layer 0)
        FbxGeometryElementVertexColor* colorElement = nullptr;
        if (mesh->GetElementVertexColorCount() > 0) {
            colorElement = mesh->GetElementVertexColor(0);
        }

        FbxGeometryElementNormal* normalElement = nullptr;
        if (mesh->GetElementNormalCount() > 0)
        {
            normalElement = mesh->GetElementNormal(0);
        }

        int polygonVertexIndex = 0;
        for (int p = 0; p < polygonCount; ++p)
        {
            // Expect polySize = 3
            const int polySize = mesh->GetPolygonSize(p);
            for (int v = 0; v < polySize; ++v)
            {
                const int controlPointIndex = mesh->GetPolygonVertex(p, v);
                Vertex vert{};
                FbxVector4 cp = controlPoints[controlPointIndex];
                vert.pos = glm::vec3(static_cast<float>(cp[0]), static_cast<float>(cp[1]), static_cast<float>(cp[2]));

                if (uvElement)
                {
                    FbxVector2 uv;
                    if (uvElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)  // Access via control point
                    {
                        int index = (uvElement->GetReferenceMode() == FbxGeometryElement::eDirect)
                            ? controlPointIndex
                            : uvElement->GetIndexArray().GetAt(controlPointIndex);
                        uv = uvElement->GetDirectArray().GetAt(index);
                    }
                    else if (uvElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex)  // Use the polygon-vertex global index
                    {
                        int index = (uvElement->GetReferenceMode() == FbxGeometryElement::eDirect)
                            ? polygonVertexIndex
                            : uvElement->GetIndexArray().GetAt(polygonVertexIndex);
                        uv = uvElement->GetDirectArray().GetAt(index);
                    }
                    else  // Other mapping modes (such as eByPolygon) are not common and are handled by default
                    {
                        uv = FbxVector2(0.0, 0.0);
                    }

                    vert.texCoord = glm::vec2(static_cast<float>(uv[0]), static_cast<float>(uv[1]));
                    if (flipV) vert.texCoord.y = 1.0f - vert.texCoord.y;
                }
                else
                {
                    vert.texCoord = glm::vec2(0.0f);
                }

                if (colorElement) {
                    FbxColor c;
                    if (colorElement->GetMappingMode() == FbxGeometryElement::eByControlPoint) 
                    {
                        int index = (colorElement->GetReferenceMode() == FbxGeometryElement::eDirect)
                            ? controlPointIndex
                            : colorElement->GetIndexArray().GetAt(controlPointIndex);
                        c = colorElement->GetDirectArray().GetAt(index);
                    }
                    else if (colorElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) 
                    {
                        int index = (colorElement->GetReferenceMode() == FbxGeometryElement::eDirect)
                            ? polygonVertexIndex
                            : colorElement->GetIndexArray().GetAt(polygonVertexIndex);
                        c = colorElement->GetDirectArray().GetAt(index);
                    }
                    else {
                        c = FbxColor(1.0, 1.0, 1.0, 1.0);
                    }
                    vert.color = glm::vec3(static_cast<float>(c.mRed), static_cast<float>(c.mGreen), static_cast<float>(c.mBlue));
                }
                else 
                {
                    vert.color = glm::vec3(1.0f, 1.0f, 1.0f);
                }

                if (normalElement)
                {
                    FbxVector4 n;
                    if (normalElement->GetMappingMode() == FbxGeometryElement::eByControlPoint)
                    {
                        int index = (normalElement->GetReferenceMode() == FbxGeometryElement::eDirect)
                            ? controlPointIndex
                            : normalElement->GetIndexArray().GetAt(controlPointIndex);
                        n = normalElement->GetDirectArray().GetAt(index);
                    }
                    else if (normalElement->GetMappingMode() == FbxGeometryElement::eByPolygonVertex) 
                    {
                        int index = (normalElement->GetReferenceMode() == FbxGeometryElement::eDirect)
                            ? polygonVertexIndex
                            : normalElement->GetIndexArray().GetAt(polygonVertexIndex);
                        n = normalElement->GetDirectArray().GetAt(index);
                    }
                    else
                    {
                        n = FbxVector4(0.0f, 1.0f, 0.0f, 1.0f);
                    }
                    vert.norm = glm::vec3(static_cast<float>(n[0]), static_cast<float>(n[1]), static_cast<float>(n[2]));
                }

                // Remove duplicates or create new vertices
                auto it = vtxToIndex.find(vert);
                if (it != vtxToIndex.end()) {
                    indices.push_back(it->second);
                }
                else {
                    uint32_t newIndex = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vert);
                    indices.push_back(newIndex);
                    vtxToIndex.emplace(vert, newIndex);
                }

                polygonVertexIndex++;
            }
        }
    }

    scene->Destroy();
    manager->Destroy();

    return true;
}

void VulkanApp::createRenderPass()
{
    vk::AttachmentDescription colorAttachment{
        .format = swapChainImageFormat,
        .samples = vk::SampleCountFlagBits::e1,
        /*
            The loadOp and storeOp determine what to do with the data in the attachment before rendering and after rendering. 
        */
        /*
            VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the attachment
            VK_ATTACHMENT_LOAD_OP_CLEAR: Clear the values to a constant at the start
            VK_ATTACHMENT_LOAD_OP_DONT_CARE: Existing contents are undefined; we don't care about them
        */
        .loadOp = vk::AttachmentLoadOp::eClear,
        /*
            VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memory and can be read later
            VK_ATTACHMENT_STORE_OP_DONT_CARE: Contents of the framebuffer will be undefined after the rendering operation
        */
        .storeOp = vk::AttachmentStoreOp::eStore,

        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,

        /*
            The initialLayout specifies which layout the image will have before the render pass begins. The finalLayout specifies the layout to automatically transition to when the render pass finishes.
        */
        /*
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Images to be presented in the swap chain
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: Images to be used as destination for a memory copy operation
        */
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR
    };

    vk::AttachmentDescription depthAttachment{
        .format = findDepthFormat(),
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
        .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal
    };

    /*
        Every subpass **references** one or more of the attachments that we've described using the structure in the previous sections.
    */

    vk::AttachmentReference colorAttachmentRef{
        .attachment = 0,
        .layout = vk::ImageLayout::eColorAttachmentOptimal
    };

    vk::AttachmentReference depthAttachmentRef{
        .attachment = 1,
        .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
    };

    vk::SubpassDependency dependency{
        .srcSubpass = vk::SubpassExternal,
        .dstSubpass = 0,
        .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eLateFragmentTests,
        .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests,
        .srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite
    };

    vk::SubpassDescription subpass{
        .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
        .colorAttachmentCount = 1,
        /*
            pInputAttachments: Attachments that are read from a shader
            pResolveAttachments: Attachments used for multisampling color attachments
            pDepthStencilAttachment: Attachment for depth and stencil data
            pPreserveAttachments: Attachments that are not used by this subpass, but for which the data must be preserved
        */
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef
    };

    std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
    vk::RenderPassCreateInfo renderPassInfo{
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    renderPass = device.createRenderPass(renderPassInfo);
}

void VulkanApp::createFramebuffers()
{
    swapChainFramebuffers.clear();
    for (size_t i = 0; i < swapChainImageViews.size(); ++i)
    {
        std::array<vk::ImageView, 2> attachments = {
            *swapChainImageViews[i],
            *depthImageView
        };
        vk::FramebufferCreateInfo framebufferInfo{
            .renderPass = renderPass,
            .attachmentCount = static_cast<uint32_t>(attachments.size()),
            .pAttachments = attachments.data(),
            .width = swapChainExtent.width,
            .height = swapChainExtent.height,
            .layers = 1
        };
        swapChainFramebuffers.push_back(std::move(device.createFramebuffer(framebufferInfo)));
    }
}

void VulkanApp::createoffScreenFramebuffer()
{
    /*
        The compromise between "picture quality/performance/compatibility" and the specific application requirements (shadow map, off-screen post-processing, texture mapping, etc.)
            shadow maps, cube map faces, GI path caching, or certain post-processing (such as the intermediate buffer of bloom) typically use fixed-resolution textures (common:) (512/1024/2048/4096), 
            because these effects have fixed resolution requirements or use power sizes to be consistent with mipmap/filtering
            The window size will change as the user makes adjustments. If the off-screen buffer is fixed to a common value (such as 2048), 
            it can avoid rebuilding resources each time the window is resized, thereby simplifying the logic/reducing jitter
    */
    offScreenFramebuffer.width = 2048;
    offScreenFramebuffer.height = 2048;

    /*
        Color attachments
    */

    // (World space) Positions
    createAttachment(vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment, &offScreenFramebuffer.position);
    // (World space) Normals
    createAttachment(vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment, &offScreenFramebuffer.normal);
    // Albedo (color)
    createAttachment(vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment, &offScreenFramebuffer.albedo);

    // Depth attachment
    auto depthFormat = findDepthFormat();
    createAttachment(depthFormat, vk::ImageUsageFlagBits::eDepthStencilAttachment, &offScreenFramebuffer.depth);

    // Set up separate renderpass with references to the color and depth attachments
    std::array<vk::AttachmentDescription, 4> attachmentDescs = {};
    for (size_t i = 0; i < 4; ++i)
    {
        attachmentDescs[i].samples = vk::SampleCountFlagBits::e1;
        attachmentDescs[i].loadOp = vk::AttachmentLoadOp::eClear;
        attachmentDescs[i].storeOp = vk::AttachmentStoreOp::eStore;
        attachmentDescs[i].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachmentDescs[i].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        if (i == 3)
        {
            attachmentDescs[i].initialLayout = vk::ImageLayout::eUndefined;
            attachmentDescs[i].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        }
        else
        {
            attachmentDescs[i].initialLayout = vk::ImageLayout::eUndefined;
            attachmentDescs[i].finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        }
    }

    // Formats
    attachmentDescs[0].format = offScreenFramebuffer.position.format;
    attachmentDescs[1].format = offScreenFramebuffer.normal.format;
    attachmentDescs[2].format = offScreenFramebuffer.albedo.format;
    attachmentDescs[3].format = offScreenFramebuffer.depth.format;

    // AttachmentReference
    std::vector<vk::AttachmentReference> colorReferences;
    colorReferences.emplace_back(0, vk::ImageLayout::eAttachmentOptimal);
    colorReferences.emplace_back(1, vk::ImageLayout::eAttachmentOptimal);
    colorReferences.emplace_back(2, vk::ImageLayout::eAttachmentOptimal);

    vk::AttachmentReference depthReference = {};
    depthReference.attachment = 3;
    depthReference.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription subpass = {};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.pColorAttachments = colorReferences.data();
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
    subpass.pDepthStencilAttachment = &depthReference;

    // Use subpass dependencies for attachment layout transitions
    std::array<vk::SubpassDependency, 3> dependencies{};

    // Depth
    dependencies[0].srcSubpass = vk::SubpassExternal;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
    dependencies[0].dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
    dependencies[0].srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    dependencies[0].dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;
    // Color
    dependencies[1].srcSubpass = vk::SubpassExternal;
    dependencies[1].dstSubpass = 0;
    dependencies[1].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    dependencies[1].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependencies[1].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
    dependencies[1].dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead;

    // Color
    dependencies[2].srcSubpass = 0;
    dependencies[2].dstSubpass = vk::SubpassExternal;
    dependencies[2].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependencies[2].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    dependencies[2].srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead;
    dependencies[2].dstAccessMask = vk::AccessFlagBits::eMemoryRead;

    vk::RenderPassCreateInfo renderPassInfo{
        .attachmentCount = static_cast<uint32_t>(attachmentDescs.size()),
        .pAttachments = attachmentDescs.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 3,
        .pDependencies = dependencies.data()
    };

    offScreenFramebuffer.renderPass = device.createRenderPass(renderPassInfo);

    std::array<vk::ImageView, 4> attachments{};
    attachments[0] = offScreenFramebuffer.position.view;
    attachments[1] = offScreenFramebuffer.normal.view;
    attachments[2] = offScreenFramebuffer.albedo.view;
    attachments[3] = offScreenFramebuffer.depth.view;

    vk::FramebufferCreateInfo framebufferCI{
        .renderPass = offScreenFramebuffer.renderPass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = offScreenFramebuffer.width,
        .height = offScreenFramebuffer.height,
        .layers = 1
    };

    offScreenFramebuffer.framebuffer = device.createFramebuffer(framebufferCI);

    vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
    vk::SamplerCreateInfo samplerInfo{
        // Magnification concerns the oversampling problem describes above, and minification concerns undersampling. 
        .magFilter = vk::Filter::eNearest,
        .minFilter = vk::Filter::eNearest,

        .mipmapMode = vk::SamplerMipmapMode::eLinear,

        /*
            VK_SAMPLER_ADDRESS_MODE_REPEAT: Repeat the texture when going beyond the image dimensions.
            VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: Like repeat, but inverts the coordinates to mirror the image when going beyond the dimensions.
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: Take the color of the edge closest to the coordinate beyond the image dimensions.
            VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: Like clamp to edge, but instead uses the edge opposite to the closest edge.
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: Return a solid color when sampling beyond the dimensions of the image.
        */
        .addressModeU = vk::SamplerAddressMode::eClampToEdge,
        .addressModeV = vk::SamplerAddressMode::eClampToEdge,
        .addressModeW = vk::SamplerAddressMode::eClampToEdge,

        .mipLodBias = 0,
        .maxAnisotropy = 1.0f,
        .minLod = 0.0f,
        .maxLod = 1.0f,
        .borderColor = vk::BorderColor::eFloatOpaqueWhite
    };
    colorSampler = device.createSampler(samplerInfo);
}

void VulkanApp::createAttachment(vk::Format format, vk::ImageUsageFlagBits usage, FramebufferAttachment* attachment)
{
    vk::ImageAspectFlags aspectMask{};

    if (usage & vk::ImageUsageFlagBits::eColorAttachment)
    {
        aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    if (usage & vk::ImageUsageFlagBits::eDepthStencilAttachment)
    {
        aspectMask = vk::ImageAspectFlagBits::eDepth;
        if (format >= vk::Format::eD16UnormS8Uint)
            aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    }

    createImage(offScreenFramebuffer.width, offScreenFramebuffer.height, format, vk::ImageTiling::eOptimal, usage | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, attachment->image, attachment->mem);
    vk::ImageViewCreateInfo imageViewCreateInfo{ 
        .viewType = vk::ImageViewType::e2D, 
        .format = format,
        .subresourceRange = { aspectMask, 0, 1, 0, 1 }
    };
    attachment->view = device.createImageView(imageViewCreateInfo);
}
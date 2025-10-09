#include <DeferredRendering/VulkanApp.h>

#include <chrono>
#include <fbxsdk.h>
#include <format>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <stb_image.h>
#include <stdexcept>

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
    auto surfaceCapabilities = deviceVK->physicalDevice.getSurfaceCapabilitiesKHR(surface);
    swapChainImageFormat = chooseSwapSurfaceFormat(deviceVK->physicalDevice.getSurfaceFormatsKHR(surface));
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
        .presentMode = chooseSwapPresentMode(deviceVK->physicalDevice.getSurfacePresentModesKHR(surface)),
        .clipped = true
    };

    swapChain = vk::raii::SwapchainKHR(deviceVK->logicDevice, SwapchainCreateInfo);
    swapChainImages = swapChain.getImages();
}

void VulkanApp::createImageViews()
{
    vk::ImageViewCreateInfo imageViewCreateInfo{ .viewType = vk::ImageViewType::e2D, .format = swapChainImageFormat,
          .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
    for (auto image : swapChainImages)
    {
        imageViewCreateInfo.image = image;
        swapChainImageViews.emplace_back(deviceVK->logicDevice, imageViewCreateInfo);
    }
}

void VulkanApp::Run()
{
    initWindow();
    initVulkan();
    // mainLoop();
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
    // createInstance();
    // setupDebugMessenger();
    // createSurface();
    // pickPhysicalDevice();
    // createLogicalDevice();
    // createSwapChain();
    // createImageViews();
    // createRenderPass();
    // createDescriptorSetLayout();
    // createGraphicPipeline();
    // createCommandPool();
    // createDepthResources();
    // createFramebuffers();
    // createTextureImage();
    // createTextureImageView();
    // createTextureSampler();
    loadModel();
    // createVertexBuffer();
    // createIndexBuffer();
    // createUniformBuffers();
    // createDescriptorPool();
    // createDescriptorSets();
    // createCommandBuffers();
    // createSyncObjects();
}

void VulkanApp::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        drawFrame();
    }

    deviceVK->logicDevice.waitIdle();
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
        deviceVK = new VulkanDevice(std::move(*devIter));
    }
    else
    {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

void VulkanApp::createLogicalDevice()
{
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = deviceVK->physicalDevice.getQueueFamilyProperties();

    // get the first index into queueFamilyProperties which supports both graphics and present
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
    {
        /*
            Any queue family with VK_QUEUE_GRAPHICS_BIT or VK_QUEUE_COMPUTE_BIT capabilities already implicitly support VK_QUEUE_TRANSFER_BIT operations.
        */
        if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
            deviceVK->physicalDevice.getSurfaceSupportKHR(qfpIndex, *surface))
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

    deviceVK->logicDevice = deviceVK->physicalDevice.createDevice(deviceCreateInfo);
    queue = deviceVK->logicDevice.getQueue(queueIndex, 0);
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
    pipelineLayout = vk::raii::PipelineLayout(deviceVK->logicDevice, pipelineLayoutInfo);

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
    pipelines.composition = deviceVK->logicDevice.createGraphicsPipeline(nullptr, pipelineCI);

    // auto bindingDescription = Vertex::getBindingDescription();
    // auto attributeDescriptions = Vertex::getAttributeDescriptions();
    
    // vk::PipelineVertexInputStateCreateInfo vertexInputCI{
    //     .vertexBindingDescriptionCount = 1,
    //     .pVertexBindingDescriptions = &bindingDescription,
    //     .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
    //     .pVertexAttributeDescriptions = attributeDescriptions.data()
    // };
    // pipelineCI.pVertexInputState = &vertexInputCI;
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

    pipelines.offscreen = deviceVK->logicDevice.createGraphicsPipeline(nullptr, pipelineCI);
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

    vk::raii::ShaderModule shaderModule{ deviceVK->logicDevice, createInfo };

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
    deviceVK->commandPool = vk::raii::CommandPool(deviceVK->logicDevice, poolInfo);
}

void VulkanApp::createCommandBuffers()
{
    commandBuffers.clear();
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = deviceVK->commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };

    commandBuffers = vk::raii::CommandBuffers(deviceVK->logicDevice, allocInfo);
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
        Since MAX_FRAMES_IN_FLIGHT is greater than 1, when the CPU is preparing for the next frame, the GPU is processing the previous frame, while deviceVK->logicDevice.waitForFences checks the fence of the current frame. 
        The fence of this frame is usually not triggered yet (because the GPU has not started processing the current frame), so the CPU will not block
    */
    while (vk::Result::eTimeout == deviceVK->logicDevice.waitForFences(*inFlightFences[currentFrame], vk::True, UINT16_MAX));

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

    deviceVK->logicDevice.resetFences(*inFlightFences[currentFrame]);
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
        presentCompleteSemaphores.emplace_back(deviceVK->logicDevice, vk::SemaphoreCreateInfo());  // Ensure that the image is obtained from the Swap Chain before the rendering queue can use the image
        renderFinishedSemaphores.emplace_back(deviceVK->logicDevice, vk::SemaphoreCreateInfo());  // Notify that the rendering of the presentation queue has been completed and images can be submitted to the screen
    }

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        inFlightFences.emplace_back(deviceVK->logicDevice, vk::FenceCreateInfo { .flags = vk::FenceCreateFlagBits::eSignaled });
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

    deviceVK->logicDevice.waitIdle();

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
}

void VulkanApp::copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size)
{
    auto commandCopyBuffer = deviceVK->beginSingleTimeCommands();
    commandCopyBuffer->copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
    deviceVK->endSingleTimeCommands(*commandCopyBuffer, queue);
}

void VulkanApp::createIndexBuffer()
{
}

void VulkanApp::createUniformBuffers()
{
    for (auto& buffer : uniformBuffers)
    {
        // Offscreen
        deviceVK->CreateBuffer(sizeof(UniformDataOffscreen), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer.offscreen.uniformBuffer, buffer.offscreen.uniformBufferMemory);
        buffer.offscreen.uniformBuffersMapped = buffer.offscreen.uniformBufferMemory.mapMemory(0, sizeof(UniformDataOffscreen));
        // Composition
        deviceVK->CreateBuffer(sizeof(UniformDataComposition), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer.composition.uniformBuffer, buffer.composition.uniformBufferMemory);
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
    descriptorSetLayout = vk::raii::DescriptorSetLayout(deviceVK->logicDevice, layoutInfo);
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
    descriptorPool = vk::raii::DescriptorPool(deviceVK->logicDevice, poolInfo);
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
        descriptorSets[i].composition = std::move(deviceVK->logicDevice.allocateDescriptorSets(allocInfo)[0]);
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
        deviceVK->logicDevice.updateDescriptorSets(writeDescriptorSets, {});

        // Offscreen (scene)
        
        // Model
        vk::DescriptorBufferInfo offscreenModelBI{
            .buffer = uniformBuffers[i].offscreen.uniformBuffer,
            .offset = 0,
            .range = sizeof(UniformDataOffscreen)
        };
        descriptorSets[i].model = std::move(deviceVK->logicDevice.allocateDescriptorSets(allocInfo)[0]);
        writeDescriptorSets = {
            // Binding 0: Vertex shader uniform buffer
            vk::WriteDescriptorSet{ .dstSet = descriptorSets[i].model, .dstBinding = 0, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &offscreenModelBI},
            // Binding 1: Color map
            vk::WriteDescriptorSet{.dstSet = descriptorSets[i].model, .dstBinding = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &modelDescriptorColorMap},
            // Binding 2: Normal map
            vk::WriteDescriptorSet{.dstSet = descriptorSets[i].model, .dstBinding = 2, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &modelDescriptorColorMap}
        };
        deviceVK->logicDevice.updateDescriptorSets(writeDescriptorSets, {});

        // Background
        descriptorSets[i].floor = std::move(deviceVK->logicDevice.allocateDescriptorSets(allocInfo)[0]);
        writeDescriptorSets = {
            // Binding 0: Vertex shader uniform buffer
            vk::WriteDescriptorSet{.dstSet = descriptorSets[i].floor, .dstBinding = 0, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &offscreenModelBI},
            // Binding 1: Color map
            vk::WriteDescriptorSet{.dstSet = descriptorSets[i].floor, .dstBinding = 1, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &bgDescriptorColorMap},
            // Binding 2: Normal map
            vk::WriteDescriptorSet{.dstSet = descriptorSets[i].floor, .dstBinding = 2, .descriptorType = vk::DescriptorType::eCombinedImageSampler, .pImageInfo = &bgDescriptorNormalMap}
        };
        deviceVK->logicDevice.updateDescriptorSets(writeDescriptorSets, {});
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
    deviceVK->CreateBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
    void* dataStaging = stagingBufferMemory.mapMemory(0, imageSize);
    memcpy(dataStaging, pixels, imageSize);
    stagingBufferMemory.unmapMemory();

    stbi_image_free(pixels);

    deviceVK->CreateImage(texWidth, texHeight, vk::Format::eR8G8B8A8Srgb, 1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage, textureImageMemory);

    deviceVK->transitionImageLayout(textureImage, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, queue);
    deviceVK->copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), queue);
    deviceVK->transitionImageLayout(textureImage, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal, queue);
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

    return vk::raii::ImageView(deviceVK->logicDevice, viewInfo);
}

void VulkanApp::createTextureSampler()
{
    vk::PhysicalDeviceProperties properties = deviceVK->physicalDevice.getProperties();
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
    textureSampler = vk::raii::Sampler(deviceVK->logicDevice, samplerInfo);
}

void VulkanApp::createDepthResources()
{
    vk::Format depthFormat = findDepthFormat();

    deviceVK->CreateImage(swapChainExtent.width, swapChainExtent.height, depthFormat, 1, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::MemoryPropertyFlagBits::eDeviceLocal, depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);
}

vk::Format VulkanApp::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features)
{
    for (const auto format : candidates) {
        vk::FormatProperties props = deviceVK->physicalDevice.getFormatProperties(format);

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

    models.model.loadFromFile(ASSETS_SRC_DIR "/Model/scifipistol/SciFiPistol.fbx", deviceVK, queue, FileLoadingFlags::PreTransformVertices | FileLoadingFlags::PreMultiplyVertexColors | FileLoadingFlags::FlipY);

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

    renderPass = deviceVK->logicDevice.createRenderPass(renderPassInfo);
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
        swapChainFramebuffers.push_back(std::move(deviceVK->logicDevice.createFramebuffer(framebufferInfo)));
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

    offScreenFramebuffer.renderPass = deviceVK->logicDevice.createRenderPass(renderPassInfo);

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

    offScreenFramebuffer.framebuffer = deviceVK->logicDevice.createFramebuffer(framebufferCI);

    vk::PhysicalDeviceProperties properties = deviceVK->physicalDevice.getProperties();
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
    colorSampler = deviceVK->logicDevice.createSampler(samplerInfo);
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

    deviceVK->CreateImage(offScreenFramebuffer.width, offScreenFramebuffer.height, format, 1, vk::ImageTiling::eOptimal, usage | vk::ImageUsageFlagBits::eSampled, vk::MemoryPropertyFlagBits::eDeviceLocal, attachment->image, attachment->mem);
    vk::ImageViewCreateInfo imageViewCreateInfo{ 
        .viewType = vk::ImageViewType::e2D, 
        .format = format,
        .subresourceRange = { aspectMask, 0, 1, 0, 1 }
    };
    attachment->view = deviceVK->logicDevice.createImageView(imageViewCreateInfo);
}
#include <HelloTriangle/HelloTriangleApp.h>
#include <chrono>
#include <format>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
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
    auto app = reinterpret_cast<HelloTriangleApp*>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

vk::Extent2D HelloTriangleApp::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
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

void HelloTriangleApp::createSwapChain()
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

void HelloTriangleApp::createImageViews()
{
    swapChainImageViews.clear();

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

    vk::ImageViewCreateInfo imageViewCreateInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = swapChainImageFormat,
        .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
    };

    for (auto image : swapChainImages)
    {
        imageViewCreateInfo.image = image;
        swapChainImageViews.emplace_back(device, imageViewCreateInfo);
    }
}

void HelloTriangleApp::Run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void HelloTriangleApp::initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

void HelloTriangleApp::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createDescriptorSetLayout();
    createGraphicPipeline();
    createCommandPool();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void HelloTriangleApp::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        drawFrame();
    }

    device.waitIdle();
}

void HelloTriangleApp::cleanup()
{
    glfwDestroyWindow(window);

    glfwTerminate();
}

void HelloTriangleApp::createInstance()
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

void HelloTriangleApp::setupDebugMessenger() {
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


std::vector<const char *> HelloTriangleApp::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers) {
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    return extensions;
}

bool HelloTriangleApp::checkValidationLayerSupport()
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
void HelloTriangleApp::pickPhysicalDevice()
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
            bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
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

void HelloTriangleApp::createLogicalDevice()
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
        {},                                                     // vk::PhysicalDeviceFeatures2
        {.synchronization2 = true, .dynamicRendering = true },  // vk::PhysicalDeviceVulkan13Features
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

void HelloTriangleApp::createSurface()
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

void HelloTriangleApp::createGraphicPipeline()
{
    //auto shaderCode = readFile("Assets/Shader/HelloTriangle/slang.spv");
    auto shaderCode = readFile(ASSETS_SRC_DIR "/Shader/HelloTriangle/slang.spv");
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

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo , fragShaderStageInfo };

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &bindingDescription,
        .vertexAttributeDescriptionCount = attributeDescriptions.size(),
        .pVertexAttributeDescriptions = attributeDescriptions.data()
    };
    /*
        The former is specified in the topology member and can have values like:
            VK_PRIMITIVE_TOPOLOGY_POINT_LIST: points from vertices
            VK_PRIMITIVE_TOPOLOGY_LINE_LIST: line from every two vertices without reuse
            VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: the end vertex of every line is used as start vertex for the next line
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: triangle from every three vertices without reuse
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: the second and third vertex of every triangle is used as first two vertices of the next triangle
    */
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{
        .topology = vk::PrimitiveTopology::eTriangleList
    };

    /*vk::Viewport viewport{
        .x = 0,
        .y = 0,
        .width = static_cast<float>(swapChainExtent.width),
        .height = static_cast<float>(swapChainExtent.height),
        .minDepth = 0,
        .maxDepth = 1
    };*/

    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    vk::PipelineViewportStateCreateInfo viewportStateInfo{ .viewportCount = 1, .scissorCount = 1 };

    vk::PipelineRasterizationStateCreateInfo rasterizerInfo{
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

    vk::PipelineColorBlendAttachmentState colorBlendAttachment{
        .blendEnable = vk::False,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eZero,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    };
    vk::PipelineColorBlendStateCreateInfo colorBlendingInfo{ 
        .logicOpEnable = vk::False, 
        .logicOp = vk::LogicOp::eCopy, 
        .attachmentCount = 1, 
        .pAttachments = &colorBlendAttachment };

    /*
        Uniform values need to be specified during pipeline creation by creating a VkPipelineLayout object. 
        Even though we won’t be using them now, we are still required to create an empty pipeline layout.
    */
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{ 
        .setLayoutCount = 1, 
        .pSetLayouts = &*descriptorSetLayout,
        .pushConstantRangeCount = 0 
    };
    pipelineLayout = vk::raii::PipelineLayout(device, pipelineLayoutInfo);

    vk::PipelineRenderingCreateInfo pipelineRenderingInfo{
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapChainImageFormat
    };

    /*
        Note that we’re using dynamic rendering instead of a traditional render pass, 
        so we set the renderPass parameter to nullptr and include a vk::PipelineRenderingCreateInfo structure in the pNext chain. 
        This structure specifies the formats of the attachments that will be used during rendering.
    */
    vk::GraphicsPipelineCreateInfo pipelineInfo{
        .pNext = &pipelineRenderingInfo,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState = &viewportStateInfo,
        .pRasterizationState = &rasterizerInfo,
        .pMultisampleState = &multisamplingInfo,
        .pColorBlendState = &colorBlendingInfo,
        .pDynamicState = &dynamicStateInfo,
        .layout = *pipelineLayout,
        /*
            Set to nullptr because we’re using dynamic rendering instead of a traditional render pass.
        */
        .renderPass = nullptr,
        /*
             Vulkan allows you to create a new graphics pipeline by deriving from an existing pipeline. 
             The idea of pipeline derivatives is that it is less expensive to set up pipelines 
             when they have much functionality in common with an existing pipeline and switching between pipelines from the same parent can also be done quicker. 
             You can either specify the handle of an existing pipeline with basePipelineHandle or reference another pipeline that is about to be created by index with basePipelineIndex.
        */
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineInfo);
}

std::vector<char> HelloTriangleApp::readFile(std::string_view filePath)
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

vk::raii::ShaderModule HelloTriangleApp::createShaderModule(const std::vector<char>& code) const
{
    vk::ShaderModuleCreateInfo createInfo{
        .codeSize = code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())
    };

    vk::raii::ShaderModule shaderModule{ device, createInfo };

    return shaderModule;
}

void HelloTriangleApp::createCommandPool()
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

void HelloTriangleApp::createCommandBuffers()
{
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };

    commandBuffers = vk::raii::CommandBuffers(device, allocInfo);
}

void HelloTriangleApp::recordCommandBuffer(uint32_t imageIndex)
{
    commandBuffers[currentFrame].begin({});

    transition_image_layout(
        imageIndex,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},  // srcAccessMask (no need to wait for previous operations)
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eTopOfPipe,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput
    );

    vk::ClearValue clearColor = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
    vk::RenderingAttachmentInfo attachmentInfo = {
        .imageView = swapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearColor
    };

    vk::RenderingInfo renderingInfo = {
        .renderArea = {.offset = { 0, 0 }, .extent = swapChainExtent },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentInfo
    };

    commandBuffers[currentFrame].beginRendering(renderingInfo);
    commandBuffers[currentFrame].bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);
    commandBuffers[currentFrame].bindVertexBuffers(0, *vertexBuffer, { 0 });
    commandBuffers[currentFrame].bindIndexBuffer(*indexBuffer, 0, vk::IndexType::eUint16);
    commandBuffers[currentFrame].setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height), 0.0f, 1.0f));
    commandBuffers[currentFrame].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent));
    commandBuffers[currentFrame].bindDescriptorSets(
        // Unlike vertex and index buffers, descriptor sets are not unique to graphics pipelines. Therefore, we need to specify if we want to bind descriptor sets to the graphics or compute pipeline. 
        vk::PipelineBindPoint::eGraphics, 
        // The layout that the descriptors are based on. 
        pipelineLayout, 
        // The index of the first descriptor set
        0, 
        *descriptorSets[currentFrame], 
        nullptr);
    //commandBuffers[currentFrame].draw(3, 1, 0, 0);
    commandBuffers[currentFrame].drawIndexed(indices.size(), 1, 0, 0, 0);
    commandBuffers[currentFrame].endRendering();
    
    transition_image_layout(
        imageIndex,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe
    );

    commandBuffers[currentFrame].end();
}

void HelloTriangleApp::transition_image_layout(uint32_t imageIndex, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags2 srcAccessMask, vk::AccessFlags2 dstAccessMask, vk::PipelineStageFlags2 srcStageMask, vk::PipelineStageFlags2 dstStageMask)
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
void HelloTriangleApp::drawFrame()
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
    result = queue.presentKHR(presentInfoKHR);
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

void HelloTriangleApp::createSyncObjects()
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

void HelloTriangleApp::recreateSwapChain()
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
}

void HelloTriangleApp::cleanupSwapChain()
{
    /*
        The disadvantage of this approach is that we need to stop all renderings before creating the new swap chain. 
        It is possible to create a new swap chain while drawing commands on an image from the old swap chain are still in-flight. 
        You need to pass the previous swap chain to the oldSwapchain field in the VkSwapchainCreateInfoKHR struct and destroy the old swap chain as soon as you’ve finished using it.
    */
    swapChainImageViews.clear();
    swapChain = nullptr;
}


/*
    Buffers in Vulkan are regions of memory used for storing arbitrary data that can be read by the graphics card. 
*/
void HelloTriangleApp::createVertexBuffer()
{
    //vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    //createBuffer(bufferSize, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, vertexBuffer, vertexBufferMemory);
    //void* data = vertexBufferMemory.mapMemory(0, bufferSize);
    ///*
    //    Unfortunately, the driver may not immediately copy the data into the buffer memory, for example, because of caching. 
    //    It is also possible that writes to the buffer are not visible in the mapped memory yet. There are two ways to deal with that problem:
    //        Use a memory heap that is host coherent, indicated with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    //        Call vkFlushMappedMemoryRanges after writing to the mapped memory, and call vkInvalidateMappedMemoryRanges before reading from the mapped memory

    //    Flushing memory ranges or using a coherent memory heap means that the driver will be aware of our writings to the buffer, but it doesn’t mean that they are actually visible on the GPU yet. 
    //    The transfer of data to the GPU is an operation that happens in the background, and the specification simply tells us that it is guaranteed to be complete as of the next call to vkQueueSubmit.
    //*/
    //memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    //vertexBufferMemory.unmapMemory();

    /*
        The memory type that allows us to access it from the CPU may not be the most optimal memory type for the graphics card itself to read from. 
        The most optimal memory has the VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT flag and is usually not accessible by the CPU on dedicated graphics cards.
        =>  One staging buffer in CPU accessible memory to upload the data from the vertex array to, and the final vertex buffer in device local memory. 
            We’ll then use a buffer copy command to move the data from the staging buffer to the actual vertex buffer.
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
uint32_t HelloTriangleApp::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties)
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

void HelloTriangleApp::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& bufferMemory)
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
        It should be noted that in a real world application, you’re not supposed to actually call vkAllocateMemory for every individual buffer. 
        The maximum number of simultaneous memory allocations is limited by the maxMemoryAllocationCount physical device limit, which may be as low as 4096 even on high end hardware like an NVIDIA GTX 1080. 
        The right way to allocate memory for a large number of objects at the same time is to create a custom allocator that splits up a single allocation among many different objects 
        by using the offset parameters that we’ve seen in many functions.

        You can either implement such an allocator yourself, or use the VulkanMemoryAllocator library provided by the GPUOpen initiative. 
        However, for this tutorial, it’s okay to use a separate allocation for every resource, because we won’t come close to hitting any of these limits for now.
    */
    bufferMemory = vk::raii::DeviceMemory(device, memoryAllocateInfo);
    buffer.bindMemory(*bufferMemory, /* the offset within the region of memory. If the offset is non-zero, then it is required to be divisible by memRequirements.alignment. */0);
}

void HelloTriangleApp::copyBuffer(vk::raii::Buffer& srcBuffer, vk::raii::Buffer& dstBuffer, vk::DeviceSize size)
{
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };
    vk::raii::CommandBuffer commandCopyBuffer = std::move(vk::raii::CommandBuffers(device, allocInfo).front());
    commandCopyBuffer.begin(vk::CommandBufferBeginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    });
    commandCopyBuffer.copyBuffer(srcBuffer, dstBuffer, vk::BufferCopy(0, 0, size));
    commandCopyBuffer.end();
    queue.submit(vk::SubmitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandCopyBuffer
    }, nullptr);
    queue.waitIdle();
}

void HelloTriangleApp::createIndexBuffer()
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


/*
    A descriptor is a way for shaders to freely access resources like buffers and images. 
    We’re going to set up a buffer that contains the transformation matrices and have the vertex shader access them through a descriptor. Usage of descriptors consists of three parts:
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
void HelloTriangleApp::createDescriptorSetLayout()
{
    vk::DescriptorSetLayoutBinding uboLayoutBinding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .pImmutableSamplers = nullptr
    };
    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .bindingCount = 1,
        .pBindings = &uboLayoutBinding
    };
    descriptorSetLayout = vk::raii::DescriptorSetLayout(device, layoutInfo);
}

void HelloTriangleApp::createUniformBuffers()
{
    uniformBuffers.clear();
    uniformBuffersMemory.clear();
    uniformBuffersMapped.clear();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DeviceSize bufferSize = sizeof(UniformBufferObject);
        vk::raii::Buffer buffer = nullptr;
        vk::raii::DeviceMemory bufferMemory = nullptr;
        createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer, bufferMemory);
        uniformBuffers.emplace_back(std::move(buffer));
        uniformBuffersMemory.emplace_back(std::move(bufferMemory));
        uniformBuffersMapped.emplace_back(uniformBuffersMemory[i].mapMemory(0, bufferSize));
    }
}

void HelloTriangleApp::updateUniformBuffer(uint32_t currentFrame)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    // The glm::rotate function takes an existing transformation, rotation angle and rotation axis as parameters.
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    // For the view transformation I’ve decided to look at the geometry from above at a 45 degree angle. The glm::lookAt function takes the eye position, center position and up axis as parameters.
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    // I’ve chosen to use a perspective projection with a 45 degree vertical field-of-view. The other parameters are the aspect ratio, near and far view planes. 
    ubo.proj = glm::perspective(glm::radians(45.0f), static_cast<float>(swapChainExtent.width) / static_cast<float>(swapChainExtent.height), 0.0f, 10.0f);
    // GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. The easiest way to compensate for that is to flip the sign on the scaling factor of the Y axis in the projection matrix. 
    // If you don’t do this, then the image will be rendered upside down.
    ubo.proj[1][1] *= -1;
    memcpy(uniformBuffersMapped[currentFrame], &ubo, sizeof(ubo));
}

void HelloTriangleApp::createDescriptorPool()
{
    vk::DescriptorPoolSize poolSize(vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT);
    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = MAX_FRAMES_IN_FLIGHT,
        .poolSizeCount = 1,
        .pPoolSizes = &poolSize
    };
    descriptorPool = vk::raii::DescriptorPool(device, poolInfo);
}

void HelloTriangleApp::createDescriptorSets()
{
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()
    };

    descriptorSets.clear();
    descriptorSets = device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vk::DescriptorBufferInfo bufferInfo{
            .buffer = uniformBuffers[i],
            .offset = 0,
            .range = sizeof(UniformBufferObject)
        };
        vk::WriteDescriptorSet descriptorWrite{
            /*
*             The first two fields specify the descriptor set to update and the binding. 
            */
            .dstSet = descriptorSets[i],
            // Remember that descriptors can be arrays, so we also need to specify the first index in the array that we want to update. 
            // We’re not using an array, so the index is simply 0.
            .dstBinding = 0,

            // It’s possible to update multiple descriptors at once in an array, starting at index dstArrayElement.
            .dstArrayElement = 0,
            // The descriptorCount field specifies how many array elements you want to update.
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            /*
                 The pBufferInfo field is used for descriptors that refer to buffer data, 
                 pImageInfo is used for descriptors that refer to image data, 
                 and pTexelBufferView is used for descriptors that refer to buffer views. 
                 Our descriptor is based on buffers, so we’re using pBufferInfo.
            */
            .pBufferInfo = &bufferInfo
        };

        device.updateDescriptorSets(descriptorWrite, {});
    }
}
#include <HelloTriangle/HelloTriangleApp.h>
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
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
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
}

void HelloTriangleApp::mainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
    }
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

void HelloTriangleApp::cleanup()
{
    glfwDestroyWindow(window);

    glfwTerminate();
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
    const auto devIter = std::ranges::find_if(devices,
        [&](auto const& device) {
            auto queueFamilies = device.getQueueFamilyProperties();
            bool supportsVulkan1_3 = device.getProperties().apiVersion >= VK_API_VERSION_1_3;
            
            const auto qfpIter = std::ranges::find_if(queueFamilies,
                [](vk::QueueFamilyProperties const& qfp) {
                    return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
                });
            bool supportsGraphics = qfpIter != queueFamilies.end();
            
            bool supportsAllRequiredExtensions = true;
            auto extensions = device.enumerateDeviceExtensionProperties();
            for (auto const& extension : deviceExtensions) {
                auto extensionIter = std::ranges::find_if(extensions, [extension](auto const& ext) {return strcmp(ext.extensionName, extension) == 0; });
                supportsAllRequiredExtensions = supportsAllRequiredExtensions && extensionIter != extensions.end();
            }

            /*
                Why needs .template?
                tell compiler '<' is start 

                vk::PhysicalDeviceVulkan13Features::dynamicRendering:
                    It allows you to directly use dynamic rendering in the command buffer - that is, you can directly specify the rendering target and subpass without having to pre-write a RenderPass in the pipeline.
                
                vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT::extendedDynamicState
                    To transform more pipeline states (such as rasterization mode, mixed state, depth/template state, etc.) into "dynamic" states, you can vkCmdSet* during drawing instead of fixing them when the pipeline is created
            */
            auto features = device.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
            bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

            return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
        });
    if (devIter == devices.end()) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
    else
    {
        physicalDevice = *devIter;
    }
}

uint32_t HelloTriangleApp::findQueueFamilies()
{
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    // get the first index into queueFamilyProperties which supports both graphics and present
    uint32_t queueIndex = ~0;
    for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
    {
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

    return queueIndex;
}

void HelloTriangleApp::createLogicalDevice()
{
    auto queueIndex = findQueueFamilies();

    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{ .queueFamilyIndex = queueIndex , .queueCount = 1, .pQueuePriorities = &queuePriority };


    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
        {},                               // vk::PhysicalDeviceFeatures2 (empty for now)
        {.dynamicRendering = true },      // Enable dynamic rendering from Vulkan 1.3
        {.extendedDynamicState = true }   // Enable extended dynamic state from the extension
    };

    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data()
    };

    /*
        Each queue family has a fixed number of available queues:
            const auto& grahicsFromProps = queueFamilyProperties[graphicsIndex];
            uint32_t maxQueues = grahicsFromProps.queueCount;
    */

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

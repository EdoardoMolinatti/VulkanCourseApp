#include "VulkanRenderer.h"

// Disable warning about Vulkan unscoped enums for this entire file
#pragma warning( push )
#pragma warning(disable : 26812) // The enum type * is unscoped. Prefer 'enum class' over 'enum'.

////////////
// Public //
////////////
//------------------------------------------------------------------------------
VulkanRenderer::VulkanRenderer()
{
}
//------------------------------------------------------------------------------
VulkanRenderer::~VulkanRenderer()
{
}
//------------------------------------------------------------------------------
// API //
//------------------------------------------------------------------------------
int VulkanRenderer::init(GLFWwindow* newWindow)
{
    m_pWindow = newWindow;

    try
    {
        createInstance();
        createDebugMessenger();
        createSurface();
        getPhysicalDevice();
        createLogicalDevice();
        createSwapchain();
    }
    catch (const std::runtime_error &e)
    {
        std::cout << "ERROR: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
//------------------------------------------------------------------------------
void VulkanRenderer::cleanup()
{
    for (auto image : m_swapchainImages)
    {
        vkDestroyImageView(m_mainDevice.logicalDevice, image.imageView, nullptr);
    }

    vkDestroySwapchainKHR(m_mainDevice.logicalDevice, m_swapChain, nullptr);

    vkDestroySurfaceKHR(m_pInstance, m_surface, nullptr);

    vkDestroyDevice(m_mainDevice.logicalDevice, nullptr);

    if (g_validationEnabled) {
        VulkanValidation::destroyDebugUtilsMessengerEXT(m_pInstance, m_debugMessenger, nullptr);
    }

    vkDestroyInstance(m_pInstance, nullptr);
}

/////////////
// Private //
/////////////
//------------------------------------------------------------------------------
void VulkanRenderer::createInstance()
{
    if (g_validationEnabled && !checkValidationLayerSupport())
    {
        throw std::runtime_error("Validation Layers requested, but at least one is not available!");
    }

    // Information about the application itself
    // Most data here doesn't affect the program and is for developer convenience
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan App";            // Custom name of the application
    appInfo.apiVersion = VK_MAKE_VERSION(0, 0, 1);      // Custom version of the application
    appInfo.pEngineName = "No Engine";                  // Custom engine name
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);   // Custom engine version
    appInfo.apiVersion = VK_API_VERSION_1_1;            // The Vulkan Version

    // Creation Information structure for a VkInstance (Vulkan Instance)
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // Optional Validation Layers
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo; // This structure must be alive (not destroyed) when calling vkCreateInstance()
    if (g_validationEnabled)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
        createInfo.ppEnabledLayerNames = g_validationLayers.data();
        std::cout    << "Added options to create " << createInfo.enabledLayerCount
                    << " Validation Layer" << ((createInfo.enabledLayerCount > 1) ? "s." : ".") << std::endl;

        // We do this to be able to debug also vkCreateInstance() and vkDestroyInstance()
        VulkanValidation::populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }

    // Retrieve the required instance extensions
    auto requiredInstanceExtensions = getRequiredInstanceExtensions();
    // Check Instance Extensions support for the required ones
    if (!checkInstanceExtensionSupport(&requiredInstanceExtensions))
    {
        throw std::runtime_error("VkInstance does not support all required extensions!");
    }
    // Add the required instance extensions to the createInfo structure
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredInstanceExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();

    // Create Vulkan Instance
    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_pInstance);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Vulkan Instance!");
    }
}
//------------------------------------------------------------------------------
void VulkanRenderer::createDebugMessenger()
{
    // Only create callback if validation enabled
    if (!g_validationEnabled) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    VulkanValidation::populateDebugMessengerCreateInfo(createInfo);

    if (VulkanValidation::createDebugUtilsMessengerEXT(m_pInstance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("Failed to set up Debug messenger!");
    }
}
//------------------------------------------------------------------------------
void VulkanRenderer::createLogicalDevice()
{
    // Get the queue family indices from the chosen Physical Device
    QueueFamilyIndices indices = getQueueFamilies(m_mainDevice.physicalDevice);

    // Vector for queue creation information and set for family indices
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

    // Queues that the logical device needs to create and infos to do so
    for (int queueFamilyIndex : queueFamilyIndices)
    {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;    // The index of the family from which create a queue
        queueCreateInfo.queueCount = 1;                         // Number of queues to create
        float priority = 1.0f;                                  // [0.0f, 1.0f] Normalized percentual from lowest to highest
        queueCreateInfo.pQueuePriorities = &priority;           // Vulkan needs to know how to handle multiple queues

        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Information to create logical device (sometimes called just "device")
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());     // Number of Queue Create Infos
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();                               // List of Queue create infos so device can create required queues
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());    // Number of enabled Logical Device Extensions
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();                         // List of enabled Logical Device Extensions

    // Physical Device Features the Logical Device will be using
    VkPhysicalDeviceFeatures deviceFeatures = {};

    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;        // Physical Device Features that Logical Device will use

    // Create the Logical Device from the given Physical Device
    VkResult result = vkCreateDevice(m_mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &m_mainDevice.logicalDevice);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Logical Device!");
    }

    // Queues are created at the same time as the device.
    // So we want handle to queues
    // From given logical device, of given Queue Family, of given Queue Index (0 since only one queue), place reference in given VkQueue
    vkGetDeviceQueue(m_mainDevice.logicalDevice, indices.graphicsFamily, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_mainDevice.logicalDevice, indices.presentationFamily, 0, &m_presentationQueue);
}
//------------------------------------------------------------------------------
void VulkanRenderer::createSurface()
{
    // Create Surface (creates a surface create info struct, runs the create surface for the specific host OS, returns result)
    VkResult result = glfwCreateWindowSurface(m_pInstance, m_pWindow, nullptr, &m_surface);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a surface!");
    }
}
//------------------------------------------------------------------------------
void VulkanRenderer::createSwapchain()
{
    // Get Swapchain details so we can pick best settings
    SwapchainDetails swapChainDetails = getSwapchainDetails(m_mainDevice.physicalDevice);

    // Find optimal surface values for our Swapchain
    VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
    VkPresentModeKHR presentationMode = chooseBestPresentationMode(swapChainDetails.presentationModes);
    VkExtent2D extent = chooseBestSwapExtent(swapChainDetails.surfaceCapabilities);

    // How many images are in the swap chain? Get 1 more than the minimum to allow triple buffering
    uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

    // If imageCount higher than max, then clamp down to max
    // If 0, then limitless
    if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 &&
        swapChainDetails.surfaceCapabilities.maxImageCount < imageCount)
    {
        imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
    }
 
    // Creation information for Swapchain
    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = m_surface;                                                    // Swapchain surface
    swapChainCreateInfo.imageFormat = surfaceFormat.format;                                     // Swapchain format
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;                             // Swapchain color space
    swapChainCreateInfo.presentMode = presentationMode;                                         // Swapchain presentation mode
    swapChainCreateInfo.imageExtent = extent;                                                   // Swapchain image extents
    swapChainCreateInfo.minImageCount = imageCount;                                             // Swapchain minimum images
    swapChainCreateInfo.imageArrayLayers = 1;                                                   // Number of layers for each image in swap chain
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;                       // What attachment images will be used as
    swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;   // Transform to perform on swap chain
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;                     // How to handle blending images with external windows
    swapChainCreateInfo.clipped = VK_TRUE;                                                      // Whether to clip parts of image not in view (e.g. behind another window)
    
    // Get Queue Family Indices
    QueueFamilyIndices indices = getQueueFamilies(m_mainDevice.physicalDevice);
    // If Graphics and Presentation families are different, then swap chain must let images be shared between families
    if (indices.graphicsFamily != indices.presentationFamily)
    {
        // Queues to share between
        uint32_t queueFamilyIndices[] = {
            static_cast<uint32_t>(indices.graphicsFamily),
            static_cast<uint32_t>(indices.presentationFamily)
        };

        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;  // Image share handling
        swapChainCreateInfo.queueFamilyIndexCount = 2;                      // Number of queues to share between
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;       // Array of queues to share between
    }
    else
    {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;   // Image share handling
        swapChainCreateInfo.queueFamilyIndexCount = 0;                      // Number of queues to share between
        swapChainCreateInfo.pQueueFamilyIndices = nullptr;                  // Array of queues to share between
    }

    // If old swap chain has been destroyed and this one replaces it, then link the old one to quickly hand over responsibilities
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    // Create Swapchain
    VkResult result = vkCreateSwapchainKHR(m_mainDevice.logicalDevice, &swapChainCreateInfo, nullptr, &m_swapChain);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Swapchain!");
    }

    // Store useful (working) values for later reference
    m_swapChainImageFormat = surfaceFormat.format;
    m_swapChainExtent = extent;

    // Get Swapchain images (first count, then values)
    uint32_t swapchainImageCount;
    vkGetSwapchainImagesKHR(m_mainDevice.logicalDevice, m_swapChain, &swapchainImageCount, nullptr);
    std::vector<VkImage> images(swapchainImageCount);
    vkGetSwapchainImagesKHR(m_mainDevice.logicalDevice, m_swapChain, &swapchainImageCount, images.data());

    // Store each Swapchain image reference in our data member
    for (VkImage image : images)
    {
        // Store image handle
        SwapchainImage swapchainImage = {};
        swapchainImage.image = image;
        swapchainImage.imageView = createImageView(image, m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

        m_swapchainImages.push_back(swapchainImage);
    }
}
//------------------------------------------------------------------------------
void VulkanRenderer::createGraphicsPipeline()
{
    // TODO
}

//------------------------------------------------------------------------------
void VulkanRenderer::getPhysicalDevice()
{
    // Enumerate Physical devices the vkInstance can access
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_pInstance, &deviceCount, nullptr);

    // If no devices available, then none support Vulkan
    if (deviceCount == 0)
    {
        throw std::runtime_error("Can't find GPUs that support Vulkan API!");
    }

    // Get list of Physical devices
    std::vector<VkPhysicalDevice> deviceList(deviceCount);
    vkEnumeratePhysicalDevices(m_pInstance, &deviceCount, deviceList.data());

    // Check for a suitable device
    for (const auto& device : deviceList)
    {
        if (checkDeviceSuitable(device))
        {
            m_mainDevice.physicalDevice = device;
            break;
        }
    }
}

//------------------------------------------------------------------------------
bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>* extensionsToCheck)
{
    VkResult vkRes = VkResult::VK_SUCCESS;

    // Need to get the number of extensions to create an array of a matching size
    uint32_t extensionsCount = 0;
    vkRes = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
    std::cout << "Vulkan Instance Extensions (available): " << extensionsCount << std::endl;

    // Create a list of VkExtensionProperties using extensionsCount
    std::vector<VkExtensionProperties> extensions(extensionsCount);
    if (extensionsCount > 0)
    {
        do
        {
            // Populate extensions vector
            VkResult vkRes = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, extensions.data());
        }
        while (vkRes == VK_INCOMPLETE);
        //std::cout << std::endl;
        //for (auto& extension : extensions) {
        //    std::cout << "   Name: " << extension.extensionName << std::endl;
        //    std::cout << "Version: " << extension.specVersion << std::endl;
        //}
    }

    // Check if given extensions are within the availables
    for (const auto& checkExtension : *extensionsToCheck)
    {
        bool hasExtension = false;
        for (const auto& extension : extensions)
        {
            if (strcmp(checkExtension, extension.extensionName) == 0)
            {
                hasExtension = true;
                break;
            }
        }

        if (!hasExtension)
        {
            std::cout << "Required Vulkan Instance extension '" << checkExtension << "' is NOT supported." << std::endl;
            return false;
        }
    }

    return true;
}
//------------------------------------------------------------------------------
bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    // Get device extensions count
    uint32_t extensionsCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr);
    std::cout << "Vulkan Device Extensions (available): " << extensionsCount << std::endl;

    // If no extensions found, return false
    if (extensionsCount <= 0)
    {
        return false;
    }

    // Create a list of VkExtensionProperties using extensionsCount
    std::vector<VkExtensionProperties> extensions(extensionsCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, extensions.data());

    // Check for device extensions
    for (const auto &deviceExtension : deviceExtensions)
    {
        bool hasExtension = false;
        for (const auto &extension : extensions)
        {
            if (strcmp(deviceExtension, extension.extensionName) == 0)
            {
                hasExtension = true;
                break;
            }
        }

        if (!hasExtension)
        {
            return false;
        }
    }

    return true;
}
//------------------------------------------------------------------------------
bool VulkanRenderer::checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : g_validationLayers)
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
        {
            return false;
        }
    }

    return true;
}
//------------------------------------------------------------------------------
bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
    /*/ Information about the device itself (ID, name, type, vendor, etc.)
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    // Information about what the device can do (geom shaders, tessellation shaders, wide lines, etc.)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
    /**/

    QueueFamilyIndices indices = getQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainValid = false;
    if (extensionsSupported)
    {
        SwapchainDetails swapChainDetails = getSwapchainDetails(device);
        swapChainValid = !swapChainDetails.formats.empty() && !swapChainDetails.presentationModes.empty();
    }

    return indices.isValid() && extensionsSupported && swapChainValid;
}

//------------------------------------------------------------------------------
std::vector<const char*> VulkanRenderer::getRequiredInstanceExtensions()
{
    // Setup the GLFW extensions that the Vulkan Instance will use
    uint32_t glfwExtensionCount = 0;    // GLFW may require multiple extensions
    const char** glfwExtensionNames;    // Extensions names passed as an array of cstrings (array of chars)

    // Get GLFW required Instance extensions
    glfwExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // Add GLFW required instance extensions to a std::vector
    std::vector<const char*> extensions(glfwExtensionNames, glfwExtensionNames + glfwExtensionCount);

    // Add also the Instance Extension required by Validation Layers, if requested
    if (g_validationEnabled) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}
//------------------------------------------------------------------------------
QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    // Get all Queue Family Property info for the given device
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

    // Go through each queue family and check if it has at least 1 of the required types of queue
    uint32_t idx = 0;
    for (const auto &queueFamily : queueFamilyList)
    {
        // First check if queue family has at least 1 queue in that family (could have no queue)
        // Queue can be multiple types defined through bitfield 'queueFlags'
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = idx;
        }

        // Check if queue families supports presentation
        VkBool32 presentationSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, idx, m_surface, &presentationSupport);
        // Check if queue is presentation type (it can be both presentation and graphics)
        if (queueFamily.queueCount > 0 && presentationSupport)
        {
            indices.presentationFamily = idx;
        }

        // Check if queue family indices are in a valid state, stop searching if so
        if (indices.isValid())
        {
            break;
        }

        idx++;
    }

    return indices;
}
//------------------------------------------------------------------------------
SwapchainDetails VulkanRenderer::getSwapchainDetails(VkPhysicalDevice device)
{
    SwapchainDetails swapChainDetails;

    // Get the Surface capabilities for the given surface on the given physical device
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &swapChainDetails.surfaceCapabilities);

    // Get the Surface formats
    uint32_t formatsCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatsCount, nullptr);

    // If at least 1 format is supported, fill the structure
    if (formatsCount > 0)
    {
        swapChainDetails.formats.resize(formatsCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatsCount, swapChainDetails.formats.data());
    }

    // Presentation modes
    uint32_t presentationCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentationCount, nullptr);

    // If at least 1 presentation mode is supported, fill the structure
    if (presentationCount > 0)
    {
        swapChainDetails.presentationModes.resize(presentationCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentationCount, swapChainDetails.presentationModes.data());
    }

    return swapChainDetails;
}

//------------------------------------------------------------------------------
VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
    // Best format is subjective, but ours will be:
    // Format:      VK_FORMAT_R8G8B8A8_UNORM or VK_FORMAT_B8G8R8A8_UNORM
    // ColorSpace:  VK_COLOR_SPACE_SRGB_NONLINEAR_KHR

    // This is a standard convention when ALL formats are supported (1 format, undefined)
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        // Default
        return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    // If not all formats are supported, then search for optimal one
    for (const auto& format : formats)
    {
        if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM) &&
            (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR))
        {
            return format;
        }
    }

    // If can't find optimal format, then just return the first one
    return formats[0];
}
//------------------------------------------------------------------------------
VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
    for (const auto& presentationMode : presentationModes)
    {
        if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return presentationMode;
        }
    }

    // According to Vulkan specifications, this one should always be available (FIFO)
    return VK_PRESENT_MODE_FIFO_KHR;
}
//------------------------------------------------------------------------------
VkExtent2D VulkanRenderer::chooseBestSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
    // If the current extent is at numeric limits, then extent can vary. Otherwise, it is the size of the window.
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return surfaceCapabilities.currentExtent;
    }
    else
    {
        // If value can vary, we need to set it manually
        int width, height;
        glfwGetFramebufferSize(m_pWindow, &width, &height);

        // Create new extent using window size
        VkExtent2D newExtent = {};
        newExtent.width = static_cast<uint32_t>(width);
        newExtent.height = static_cast<uint32_t>(height);

        // Surface also defines max and min, so make sure we're between boundaries by clamping value
        newExtent.width = std::max( surfaceCapabilities.minImageExtent.width,
                                    std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
        newExtent.height = std::max(surfaceCapabilities.minImageExtent.height,
                                    std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

        return newExtent;
    }
}

//------------------------------------------------------------------------------
VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image;                                   // Image to create the view for
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;                // Type of image
    viewCreateInfo.format = format;                                 // Format of image data
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;    // Swizzle allows RGB components to map to other values
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // Subresources allows the view to view only part of an image
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;       // Which aspect of image to view (e.g. COLOR_BIT, etc.)
    viewCreateInfo.subresourceRange.baseMipLevel = 0;               // Start mipmap level to view from
    viewCreateInfo.subresourceRange.levelCount = 1;                 // Number of mipmap levels to view
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;             // Start layer to view from
    viewCreateInfo.subresourceRange.layerCount = 1;                 // Number of array levels to view

    // Create image view and return it
    VkImageView imageView;
    VkResult result = vkCreateImageView(m_mainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create ImageView!");
    }
    return imageView;
}
//------------------------------------------------------------------------------
VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
{
    // TODO
    return VkShaderModule();
}

#pragma warning( pop )

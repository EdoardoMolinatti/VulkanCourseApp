#include "VulkanRenderer.h"

// C++ STL
#include <chrono>
#include <thread>

using std::cout;
using std::endl;

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
        createRenderPass();
        createDescriptorSetLayout();
        createPushConstantRange();
        createGraphicsPipeline();
        createDepthBufferImage();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createTextureSampler();
        //allocateDynamicBufferTransferSpace();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
        createSynchronisation();

        //======================================================================
        //------------------------------
        // Model-View-Projection setup
        //------------------------------
        m_uboViewProjection.projection = glm::perspective(
            glm::radians(45.0f), static_cast<float>(m_swapChainExtent.width) / static_cast<float>(m_swapChainExtent.height), 0.1f, 100.0f);
        //               FOV-Y ,                                       Aspect Ratio                                        ,zNear, zFar
        m_uboViewProjection.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        //                                 Eye              ,           Center           ,           Up

        m_uboViewProjection.projection[1][1] *= -1; // Vulkan inverts Y coordinates compared to OpenGL (and GLM is based upon OpenGL coordinate system)

        //------------------------------
        // Create meshes
        //------------------------------
        // Vertex Data
        std::vector<Vertex> meshVertices1 = {
            { { -0.4,  0.4, 0.0 }, { 1.0f, 0.0f, 0.0f }, {0.0f, 0.0f} },    // 0
            { { -0.4, -0.4, 0.0 }, { 1.0f, 0.0f, 0.0f }, {0.0f, 1.0f} },    // 1
            { {  0.4, -0.4, 0.0 }, { 1.0f, 0.0f, 0.0f }, {1.0f, 1.0f} },    // 2
            { {  0.4,  0.4, 0.0 }, { 1.0f, 0.0f, 0.0f }, {1.0f, 0.0f} },    // 3
        };

        std::vector<Vertex> meshVertices2 = {
            { { -0.57,  0.6, 0.0 }, { 0.0f, 0.0f, 1.0f }, {0.0f, 0.0f} },    // 0
            { { -0.57, -0.6, 0.0 }, { 0.0f, 0.0f, 1.0f }, {0.0f, 1.0f} },    // 1
            { {  0.57, -0.6, 0.0 }, { 0.0f, 0.0f, 1.0f }, {1.0f, 1.0f} },    // 2
            { {  0.57,  0.6, 0.0 }, { 0.0f, 0.0f, 1.0f }, {1.0f, 0.0f} },    // 3
        };

        // Index Data
        std::vector<uint32_t> meshIndices = {
            0, 1, 2, 
            2, 3, 0
        };    

        Mesh firstMesh = Mesh(m_mainDevice.physicalDevice, m_mainDevice.logicalDevice,
            m_graphicsQueue, m_graphicsCommandPool,
            &meshVertices1, &meshIndices,
            createTexture("giraffe.jpg"));
        Mesh secondMesh = Mesh(m_mainDevice.physicalDevice, m_mainDevice.logicalDevice,
            m_graphicsQueue, m_graphicsCommandPool,
            &meshVertices2, &meshIndices,
            createTexture("panda.jpg"));

        m_meshList.push_back(firstMesh);
        m_meshList.push_back(secondMesh);

        glm::mat4 meshModelMatrix = m_meshList[0].getModel().model;
        meshModelMatrix = glm::rotate( meshModelMatrix, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f) );
        m_meshList[0].setModel(meshModelMatrix);
        //======================================================================
    }
    catch (const std::runtime_error &e)
    {
        cout << "ERROR: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
//------------------------------------------------------------------------------
bool VulkanRenderer::isWindowIconified()
{
    if (glfwGetWindowAttrib(m_pWindow, GLFW_ICONIFIED))
    {
        return true;
    }
    return false;
}
//------------------------------------------------------------------------------
bool VulkanRenderer::updateModel(uint32_t modelId, glm::mat4 modelMatrix)
{
    if (modelId >= m_meshList.size()) { return false; }

    m_meshList[modelId].setModel(modelMatrix);
    return true;
}
//------------------------------------------------------------------------------
void VulkanRenderer::draw(double frameDuration)
{
    // Check if the window is iconified
    if (glfwGetWindowAttrib(m_pWindow, GLFW_ICONIFIED))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<uint32_t>(frameDuration)));
        return;
    }

    // Wait for given fence to signal (open) from last draw before continuing
    vkWaitForFences(m_mainDevice.logicalDevice, 1, &m_drawFences[m_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
    // Manually reset (close) fences
    vkResetFences(m_mainDevice.logicalDevice, 1, &m_drawFences[m_currentFrame]);

    // -- GET NEXT IMAGE --
    // Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
    uint32_t imageIndex;
    vkAcquireNextImageKHR(m_mainDevice.logicalDevice, m_swapChain, std::numeric_limits<uint64_t>::max(), m_imageAvailable[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

    recordCommands(imageIndex);

    // Update Uniform Buffer (this should be after the acquiring of next image)
    updateUniformBuffers(imageIndex);
    
    // -- SUBMIT COMMAND BUFFER TO RENDER --
    // Queue submission information
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;                                  // Number of semaphores to wait on
    submitInfo.pWaitSemaphores = &m_imageAvailable[m_currentFrame];     // List of semaphores to wait on
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    submitInfo.pWaitDstStageMask = waitStages;                          // Stages to check semaphores at
    submitInfo.commandBufferCount = 1;                                  // Number of command buffers to submit
    submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];         // Command buffer to submit
    submitInfo.signalSemaphoreCount = 1;                                // Number of semaphores to signal
    submitInfo.pSignalSemaphores = &m_renderFinished[m_currentFrame];   // Semaphores to signal when command buffer finishes

    // Submit command buffer to queue (N.B.: queues are like conveyor belts, always running)
    VkResult result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_drawFences[m_currentFrame]);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit Command Buffer to Queue!");
    }


    // -- PRESENT RENDERED IMAGE TO SCREEN --
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;                                 // Number of semaphores to wait on
    presentInfo.pWaitSemaphores = &m_renderFinished[m_currentFrame];    // Semaphores to wait on
    presentInfo.swapchainCount = 1;                                     // Number of swapchains to present to
    presentInfo.pSwapchains = &m_swapChain;                             // Swapchains to present images to
    presentInfo.pImageIndices = &imageIndex;                            // Index of images in swapchains to present

    // Present image (to screen - render the processed image)
    result = vkQueuePresentKHR(m_presentationQueue, &presentInfo);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present Image!");
    }

    // Get next frame (use % MAX_FRAME_DRAWS to keep value below MAX_FRAME_DRAWS)
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAME_DRAWS;
}
//------------------------------------------------------------------------------
void VulkanRenderer::cleanup()
{
    // Wait until no actions being run on device before destroying
    vkDeviceWaitIdle(m_mainDevice.logicalDevice);

    // Free the aligned memory used for Dynamic Uniform Buffers
    //_aligned_free(m_pModelTransferSpace);

    // Destroy Textures (Descriptors + Sampler)
    vkDestroyDescriptorPool(m_mainDevice.logicalDevice, m_samplerDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_mainDevice.logicalDevice, m_samplerSetLayout, nullptr);

    vkDestroySampler(m_mainDevice.logicalDevice, m_textureSampler, nullptr);

    // Destroy Textures (ImageView + Image + Memory)
    for (size_t i = 0; i < m_textureImages.size(); ++i)
    {
        vkDestroyImageView(m_mainDevice.logicalDevice, m_textureImageViews[i], nullptr);
        vkDestroyImage(m_mainDevice.logicalDevice, m_textureImages[i], nullptr);
        vkFreeMemory(m_mainDevice.logicalDevice, m_textureImageMemory[i], nullptr);
    }

    // Destroy Depth Buffer ImageView, Image and related video memory
    vkDestroyImageView(m_mainDevice.logicalDevice, m_depthBufferImageView, nullptr);
    vkDestroyImage(m_mainDevice.logicalDevice, m_depthBufferImage, nullptr);
    vkFreeMemory(m_mainDevice.logicalDevice, m_depthBufferImageMemory, nullptr);

    // Destroy Descriptor Pool and Descriptor SetLayout
    vkDestroyDescriptorPool(m_mainDevice.logicalDevice, m_descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(m_mainDevice.logicalDevice, m_descriptorSetLayout, nullptr);
    // Destroy Uniform Buffers and free related memory
    for (size_t i = 0; i < m_vpUniformBuffer.size(); ++i)
    {
        vkDestroyBuffer(m_mainDevice.logicalDevice, m_vpUniformBuffer[i], nullptr);
        vkFreeMemory(m_mainDevice.logicalDevice, m_vpUniformBufferMemory[i], nullptr);
        //vkDestroyBuffer(m_mainDevice.logicalDevice, m_modelDynUniformBuffer[i], nullptr);
        //vkFreeMemory(m_mainDevice.logicalDevice, m_modelDynUniformBufferMemory[i], nullptr);
    }

    // Destroy Meshes
    for (size_t i = 0; i < m_meshList.size(); ++i)
    {
        m_meshList[i].destroyBuffers();
    }

    // Destroy Synchronization structures
    for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
        vkDestroySemaphore(m_mainDevice.logicalDevice, m_renderFinished[i], nullptr);
        vkDestroySemaphore(m_mainDevice.logicalDevice, m_imageAvailable[i], nullptr);
        vkDestroyFence(m_mainDevice.logicalDevice, m_drawFences[i], nullptr);
    }

    vkDestroyCommandPool(m_mainDevice.logicalDevice, m_graphicsCommandPool, nullptr);

    // Destroy Swapchain buffers
    for (auto framebuffer : m_swapChainFramebuffers)
    {
        vkDestroyFramebuffer(m_mainDevice.logicalDevice, framebuffer, nullptr);
    }

    // Destroy Pipeline and RenderPass
    vkDestroyPipeline(m_mainDevice.logicalDevice, m_graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(m_mainDevice.logicalDevice, m_pipelineLayout, nullptr);
    vkDestroyRenderPass(m_mainDevice.logicalDevice, m_renderPass, nullptr);

    // Destroy the image views, Swapchain and Surface
    for (auto &image : m_swapchainImages)
    {
        vkDestroyImageView(m_mainDevice.logicalDevice, image.imageView, nullptr);
    }
    vkDestroySwapchainKHR(m_mainDevice.logicalDevice, m_swapChain, nullptr);
    vkDestroySurfaceKHR(m_pInstance, m_surface, nullptr);

    // Destroy the Vulkan Device
    vkDestroyDevice(m_mainDevice.logicalDevice, nullptr);

    if (sg_validationEnabled)
    {
        VulkanValidation::destroyDebugUtilsMessengerEXT(m_pInstance, m_debugMessenger, nullptr);
    }

    // Finally, destroy the Vulkan Instance
    vkDestroyInstance(m_pInstance, nullptr);
}

/////////////
// Private //
/////////////
//------------------------------------------------------------------------------
void VulkanRenderer::createInstance()
{
#pragma warning( push )
#pragma warning(disable : 6237) // (<zero> && <expression>) is always zero. <expression> is never evaluated and may have side effects
    // Optional Validation Layers
    if (sg_validationEnabled && !checkValidationLayerSupport())
    {
        throw std::runtime_error("Validation Layers requested, but at least one of them isn't available!");
    }
#pragma warning( pop )

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
    if (sg_validationEnabled)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(g_validationLayers.size());
        createInfo.ppEnabledLayerNames = g_validationLayers.data();
        cout    << "Added options to create " << createInfo.enabledLayerCount
                << " Validation Layer" << ((createInfo.enabledLayerCount > 1) ? "s." : ".") << endl;

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

    // Vulkan Version
    uint32_t vkApiVersion;
    vkEnumerateInstanceVersion(&vkApiVersion);
    cout << "Vulkan Instance Version: " << getVersionString(vkApiVersion) << endl;
}
//------------------------------------------------------------------------------
void VulkanRenderer::createDebugMessenger()
{
    // Only create callback if validation enabled
    if (!sg_validationEnabled) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo {};
    VulkanValidation::populateDebugMessengerCreateInfo(createInfo);

    if (VulkanValidation::createDebugUtilsMessengerEXT(m_pInstance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS)
    {
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
    deviceFeatures.samplerAnisotropy = VK_TRUE;                 // Enable Anisotropy

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
void VulkanRenderer::createRenderPass()
{
    // Colour attachment of render pass (index: 0)
    VkAttachmentDescription colourAttachment = {};
    colourAttachment.format = m_swapChainImageFormat;                   // Format to use for attachment
    colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;                   // Number of samples to write for multisampling
    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;              // Describes what to do with attachment before rendering
    colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;            // Describes what to do with attachment after rendering
    colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;   // Describes what to do with stencil before rendering
    colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Describes what to do with stencil after rendering

    // Framebuffer data will be stored as an image, but images can be given different data layouts
    // to give optimal use for certain operations
    colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;         // Image data layout before render pass starts
    colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;     // Image data layout after render pass (to change to)

    // Depth attachment of render pass
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = chooseSupportedFormat(
        { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Attachment reference uses an attachment index that refers to index in the attachment array passed to renderPassCreateInfo
    VkAttachmentReference colourAttachmentReference = {};
    colourAttachmentReference.attachment = 0;                           // Colour attachment index
    colourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth Attachment Reference
    VkAttachmentReference depthAttachmentReference = {};
    depthAttachmentReference.attachment = 1;                            // Depth attachment index
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Information about a particular subpass the Render Pass is using
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;        // Pipeline type subpass is to be bound to
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colourAttachmentReference;
    subpass.pDepthStencilAttachment = &depthAttachmentReference;

    // Need to determine when layout transitions occur using subpass dependencies
    std::array<VkSubpassDependency, 2> subpassDependencies = {};

    // Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    // Transition must happen after (source moment)...
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;                        // Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;     // Pipeline stage
    subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;               // Stage access mask (memory access)
    // But must happen before (destination moment)...
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dependencyFlags = 0;

    // Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    // Transition must happen after...
    subpassDependencies[1].srcSubpass = 0;
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;;
    // But must happen before...
    subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependencies[1].dependencyFlags = 0;

    std::array<VkAttachmentDescription, 2> renderPassAttachments = { colourAttachment, depthAttachment };

    // Create info for Render Pass
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
    renderPassCreateInfo.pAttachments = renderPassAttachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
    renderPassCreateInfo.pDependencies = subpassDependencies.data();

    VkResult result = vkCreateRenderPass(m_mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &m_renderPass);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Render Pass!");
    }
}
//------------------------------------------------------------------------------
void VulkanRenderer::createDescriptorSetLayout()
{
    // UNIFORM VALUES DESCRIPTOR SET LAYOUT
    // VP (View-Projection) Binding Info
    VkDescriptorSetLayoutBinding vpLayoutBinding = {};
    vpLayoutBinding.binding = 0;                                           // Binding point in shader (designated by binding number in shader)
    vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;    // Type of descriptor (uniform, dynamic uniform, image sampler, etc)
    vpLayoutBinding.descriptorCount = 1;                                   // Number of descriptors (in the shader) for binding
    vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;               // Shader stage to bind to (vertex shader in this case)
    vpLayoutBinding.pImmutableSamplers = nullptr;                          // For Texture: Can make sampler data unchangeable (immutable) by specifying in layout
    // N.B.: This is Set 0, Binding 0

    // M (Model) Binding Info
    //VkDescriptorSetLayoutBinding modelLayoutBinding = {};
    //modelLayoutBinding.binding = 1;
    //modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    //modelLayoutBinding.descriptorCount = 1;
    //modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    //modelLayoutBinding.pImmutableSamplers = nullptr;

    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { vpLayoutBinding };

    // Create Descriptor Set Layout with given bindings
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());   // Number of binding infos
    layoutCreateInfo.pBindings = layoutBindings.data();                             // Array of binding infos

    // Create Descriptor Set Layout
    VkResult result = vkCreateDescriptorSetLayout(m_mainDevice.logicalDevice, &layoutCreateInfo, nullptr, &m_descriptorSetLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Descriptor Set Layout!");
    }

    // CREATE TEXTURE SAMPLER DESCRIPTOR SET LAYOUT
    // Texture binding info
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 0;                                                   // Binding point in shader (designated by binding number in shader)
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;    // Type of descriptor (uniform, dynamic uniform, image sampler, etc)
    samplerLayoutBinding.descriptorCount = 1;                                           // Number of descriptors (in the shader) for binding
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;                     // Shader stage to bind to (vertex shader in this case)
    samplerLayoutBinding.pImmutableSamplers = nullptr;                                  // For Texture: Can make sampler data unchangeable (immutable) by specifying in layout
    // N.B.: This is Set 1, Binding 0

    // Create a Descriptor Set Layout with given bindings for texture
    VkDescriptorSetLayoutCreateInfo textureLayoutCreateInfo = {};
    textureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutCreateInfo.bindingCount = 1;
    textureLayoutCreateInfo.pBindings = &samplerLayoutBinding;

    // Create Descriptor Set Layout
    result = vkCreateDescriptorSetLayout(m_mainDevice.logicalDevice, &textureLayoutCreateInfo, nullptr, &m_samplerSetLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Descriptor Set Layout!");
    }
}
//------------------------------------------------------------------------------
void VulkanRenderer::createPushConstantRange()
{
    // Define push constant value (no 'create' needed!)
    m_pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;    // Shader stage push constant will go to
    m_pushConstantRange.offset = 0;                                 // Offset of the start of data
    m_pushConstantRange.size = sizeof(Model);
}
//------------------------------------------------------------------------------
void VulkanRenderer::createGraphicsPipeline()
{
    // TODO: compile shaders in another ad hoc method and call it just in debug configuration
    // Read in the SPIR-V binary code of the shaders
    auto vertexShaderCode = readBinaryFile("Shaders/vert.spv");
    auto fragmentShaderCode = readBinaryFile("Shaders/frag.spv");

    // |A| Create Shader Modules (ALWAYS keep sure to destroy them to avoid memory leaks)
    VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

    // -- SHADER STAGE CREATION INFORMATION --
    // Vertex Stage creation information
    VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;          // Shader Stage name
    vertexShaderCreateInfo.module = vertexShaderModule;                 // Shader module to be used by stage
    vertexShaderCreateInfo.pName = "main";                              // Entry point function name (in the shader)

    // Fragment Stage creation information
    VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
    fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;      // Shader Stage name
    fragmentShaderCreateInfo.module = fragmentShaderModule;             // Shader module to be used by stage
    fragmentShaderCreateInfo.pName = "main";                            // Entry point function name (in the shader)

    // Put shader stage creation info in to array
    // Graphics Pipeline creation info requires array of shader stage creates
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

    // CREATE GRAPHICS PIPELINE
    // Vertex binding description (including info such as position, colour, texture coords, normals, etc) as a whole
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;                                 // Can bind multiple streams of data, this defines which one
    bindingDescription.stride = sizeof(Vertex);                     // Size of a single vertex object
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;     // How to move between data after each vertex.
                                                                    // VK_VERTEX_INPUT_RATE_INDEX        : Move on to the next vertex
                                                                    // VK_VERTEX_INPUT_RATE_INSTANCE    : Move to a vertex for the next instance

    // How the data for an attribute is defined within a vertex
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

    // Vertex Position Attribute
    attributeDescriptions[0].binding = 0;                           // Which binding the data is at (should be same as above)
    attributeDescriptions[0].location = 0;                          // Location in shader where data will be read from
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;   // Format the data will take (also helps define size of data)
    attributeDescriptions[0].offset = offsetof(Vertex, pos);        // Where this attribute is defined in the data for a single vertex

    // Vertex Colour Attribute
    attributeDescriptions[1].binding = 0;                            
    attributeDescriptions[1].location = 1;                            
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;    
    attributeDescriptions[1].offset = offsetof(Vertex, col);

    // Vertex Texture Attribute
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, tex);

    // -- VERTEX INPUT --
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;             // List of Vertex Binding Descriptions (data spacing/stride information)
    vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();  // List of Vertex Attribute Descriptions (data format and where to bind to/from)


    // -- INPUT ASSEMBLY --
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;   // Primitive type to assemble vertices as
    inputAssembly.primitiveRestartEnable = VK_FALSE;                // Allow overriding of "strip" topology to start new primitives


    // -- VIEWPORT & SCISSOR --
    // Create a viewport info struct
    VkViewport viewport = {};
    viewport.x = 0.0f;                                              // x start coordinate
    viewport.y = 0.0f;                                              // y start coordinate
    viewport.width = static_cast<float>(m_swapChainExtent.width);   // width of viewport
    viewport.height = static_cast<float>(m_swapChainExtent.height); // height of viewport
    viewport.minDepth = 0.0f;                                       // min framebuffer depth
    viewport.maxDepth = 1.0f;                                       // max framebuffer depth

    // Create a scissor info struct
    VkRect2D scissor = {};
    scissor.offset = { 0,0 };                                       // Offset to use region from
    scissor.extent = m_swapChainExtent;                             // Extent to describe region to use, starting at offset

    // Viewport State info struct
    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    // -- DYNAMIC VIEWPORT STATES --
    // Dynamic states to enable resizing. Remember to also delete and re-create a SwapChain for the new resolution!
    //std::vector<VkDynamicState> dynamicStateEnables;
    //dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT); // Dynamic Viewport : Can resize in command buffer with vkCmdSetViewport(commandbuffer, 0, 1, &viewport);
    //dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);  // Dynamic Scissor  : Can resize in command buffer with vkCmdSetScissor(commandbuffer, 0, 1, &scissor);

    // Dynamic State creation info
    //VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    //dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    //dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
    //dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();


    // -- RASTERIZER --
    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCreateInfo.depthClampEnable = VK_FALSE;                   // Change if fragments beyond near/far planes are clipped (default) or clamped to plane
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;            // Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;            // How to handle filling points between vertices (if not fill, check for the feature needed for that)
    rasterizerCreateInfo.lineWidth = 1.0f;                              // How thick lines should be when drawn
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;              // Which face of a triangle to cull
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;   // Winding to determine which side is front
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;                    // Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)


    // -- MULTISAMPLING --
    VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
    multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;                 // Enable multisample shading or not
    multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;   // Number of samples to use per fragment


    // -- BLENDING --
    // Blending decides how to blend a new colour being written to a fragment, with the old value

    // Blend Attachment State (how blending is handled)
    VkPipelineColorBlendAttachmentState colourState = {};
    colourState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT    // Colours to apply blending to
        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colourState.blendEnable = VK_TRUE;                                                  // Enable blending

    // Blending uses equation: (srcColorBlendFactor * new colour) colorBlendOp (dstColorBlendFactor * old colour)
    colourState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colourState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colourState.colorBlendOp = VK_BLEND_OP_ADD;
    // Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new colour) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
    //                      (new colour alpha * new colour) + ((1 - new colour alpha) * old colour)

    colourState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colourState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colourState.alphaBlendOp = VK_BLEND_OP_ADD;
    // Summarised: (1 * new alpha) + (0 * old alpha) = new alpha

    VkPipelineColorBlendStateCreateInfo colourBlendingCreateInfo = {};
    colourBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colourBlendingCreateInfo.logicOpEnable = VK_FALSE;      // Alternative to calculations (colourState) is to use logical operations
    colourBlendingCreateInfo.attachmentCount = 1;
    colourBlendingCreateInfo.pAttachments = &colourState;

    // -- PIPELINE LAYOUT --
    std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { m_descriptorSetLayout, m_samplerSetLayout };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &m_pushConstantRange;

    // Create Pipeline Layout
    VkResult result = vkCreatePipelineLayout(m_mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Pipeline Layout!");
    }


    // -- DEPTH STENCIL TESTING --
    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
    depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;               // Enable checking depth to determine fragment write
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;              // Enable writing to depth buffer (to replace old values)
    depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;     // Comparison operation that allows an overwrite (if it's in front)
    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;        // Depth Bounds Test: Does the depth value exist between two bounds
    depthStencilCreateInfo.stencilTestEnable = VK_FALSE;            // Enable Stencil test


    // -- GRAPHICS PIPELINE CREATION --
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = ARRAY_SIZE(shaderStages);           // Number of shader stages
    pipelineCreateInfo.pStages = shaderStages;                          // List of shader stages
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;      // All the fixed function pipeline states
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colourBlendingCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
    pipelineCreateInfo.layout = m_pipelineLayout;                       // Pipeline Layout pipeline should use
    pipelineCreateInfo.renderPass = m_renderPass;                       // Render pass the pipeline is compatible with
    pipelineCreateInfo.subpass = 0;                                     // Subpass index of render pass to use with pipeline

    // Pipeline Derivatives: can create multiple pipelines that derive from one another for optimisation
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;             // Existing pipeline to derive from...
    pipelineCreateInfo.basePipelineIndex = -1;                          // or index of pipeline being created to derive from (in case creating multiple at once)

    // Create Graphics Pipeline
    result = vkCreateGraphicsPipelines(m_mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_graphicsPipeline);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Graphics Pipeline!");
    }

    // |B| Destroy Shader Modules, no longer needed after the Pipeline is created
    vkDestroyShaderModule(m_mainDevice.logicalDevice, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(m_mainDevice.logicalDevice, vertexShaderModule, nullptr);
}
//------------------------------------------------------------------------------
void VulkanRenderer::createDepthBufferImage()
{
    // Get supported format for depth buffer
    VkFormat depthFormat = chooseSupportedFormat(
        { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // Create Depth Buffer Image
    m_depthBufferImage = createImage(m_swapChainExtent.width, m_swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_depthBufferImageMemory);

    // Create Depth Buffer Image View
    m_depthBufferImageView = createImageView(m_depthBufferImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}
//------------------------------------------------------------------------------
void VulkanRenderer::createFramebuffers()
{
    // Resize framebuffer count to equal swap chain image count
    m_swapChainFramebuffers.resize(m_swapchainImages.size());

    // Create a framebuffer for each swap chain image
    for (size_t i = 0; i < m_swapChainFramebuffers.size(); ++i)
    {
        std::array<VkImageView, 2> attachments = {
            m_swapchainImages[i].imageView,
            m_depthBufferImageView
        };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = m_renderPass;                                    // Render Pass layout the Framebuffer will be used with
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferCreateInfo.pAttachments = attachments.data();                            // List of attachments (1:1 with Render Pass)
        framebufferCreateInfo.width = m_swapChainExtent.width;                              // Framebuffer width
        framebufferCreateInfo.height = m_swapChainExtent.height;                            // Framebuffer height
        framebufferCreateInfo.layers = 1;                                                   // Framebuffer layers

        VkResult result = vkCreateFramebuffer(m_mainDevice.logicalDevice, &framebufferCreateInfo, nullptr, &m_swapChainFramebuffers[i]);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a Framebuffer!");
        }
    }
}
//------------------------------------------------------------------------------
void VulkanRenderer::createCommandPool()
{
    // Get indices of queue families from device
    QueueFamilyIndices queueFamilyIndices = getQueueFamilies(m_mainDevice.physicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;   // This automatically resets the command buffer for each frame draw
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;      // Queue Family type that buffers from this command pool will use

    // Create a Graphics Queue Family Command Pool
    VkResult result = vkCreateCommandPool(m_mainDevice.logicalDevice, &poolInfo, nullptr, &m_graphicsCommandPool);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Command Pool!");
    }
}
//------------------------------------------------------------------------------
void VulkanRenderer::createCommandBuffers()
{
    // Resize command buffer count to have one for each framebuffer
    m_commandBuffers.resize(m_swapChainFramebuffers.size());

    // N.B.: Not a Create but Allocate, because CommandBuffers are already there, we are just allocating them
    VkCommandBufferAllocateInfo cbAllocateInfo = {};
    cbAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocateInfo.commandPool = m_graphicsCommandPool;
    cbAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // VK_COMMAND_BUFFER_LEVEL_PRIMARY  : Buffer you submit directly to queue, it'll be executed. Can't be called by other buffers.
                                                            // VK_COMMAND_BUFFER_LEVEL_SECONDARY: Buffer can't be called directly. Can be called from other buffers
                                                            //                                    via "vkCmdExecuteCommands" when recording commands in primary buffer.
    cbAllocateInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

    // Allocate command buffers and place handles in array of buffers
    VkResult result = vkAllocateCommandBuffers(m_mainDevice.logicalDevice, &cbAllocateInfo, m_commandBuffers.data());
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Command Buffers!");
    }
}
//------------------------------------------------------------------------------
void VulkanRenderer::createSynchronisation()
{
    m_imageAvailable.resize(MAX_FRAME_DRAWS);
    m_renderFinished.resize(MAX_FRAME_DRAWS);
    m_drawFences.resize(MAX_FRAME_DRAWS);

    // Semaphore (GPU-GPU) creation information
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Fence (GPU-CPU) creation information
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;           // This one should start signaled (open)

    for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
        if (vkCreateSemaphore(m_mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &m_imageAvailable[i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &m_renderFinished[i]) != VK_SUCCESS ||
            vkCreateFence(m_mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &m_drawFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a Semaphore and/or Fence!");
        }
    }
}

void VulkanRenderer::createTextureSampler()
{
    // Sampler Creation Info
    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR; //TODO: try VK_FILTER_CUBIC_IMG                  // How to render when image is magnified on screen
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;                     // How to render when image is minified on screen
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;    // How to handle texture wrap in U (x) direction
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;    // How to handle texture wrap in V (y) direction
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;    // How to handle texture wrap in W (z) direction
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;   // Border beyond texture (only workds for border clamp)
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;               // Whether coords should be normalized (between 0 and 1)
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;       // Mipmap interpolation mode
    samplerCreateInfo.mipLodBias = 0.0f;                                // Level of Details bias for mip level
    samplerCreateInfo.minLod = 0.0f;                                    // Minimum Level of Detail to pick mip level
    samplerCreateInfo.maxLod = 0.0f;                                    // Maximum Level of Detail to pick mip level
    samplerCreateInfo.anisotropyEnable = VK_TRUE;                       // Enable Anisotropy
    samplerCreateInfo.maxAnisotropy = 16;                               // Anisotropy sample level

    VkResult result = vkCreateSampler(m_mainDevice.logicalDevice, &samplerCreateInfo, nullptr, &m_textureSampler);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Filed to create a Texture Sampler!");
    }
}

//------------------------------------------------------------------------------
void VulkanRenderer::createUniformBuffers()
{
    // ViewProjection buffer size
    VkDeviceSize vpBufferSize = sizeof(UboViewProjection);

    // Model buffer size
    //VkDeviceSize modelBufferSize = m_modelUniformAlignment * MAX_OBJECTS;

    // One uniform buffer for each image (and by extension, command buffer)
    m_vpUniformBuffer.resize(m_swapchainImages.size());
    m_vpUniformBufferMemory.resize(m_swapchainImages.size());
    //m_modelDynUniformBuffer.resize(m_swapchainImages.size());
    //m_modelDynUniformBufferMemory.resize(m_swapchainImages.size());

    // Create Uniform buffers
    for (size_t i = 0; i < m_swapchainImages.size(); ++i)
    {
        createBuffer(m_mainDevice.physicalDevice, m_mainDevice.logicalDevice, vpBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_vpUniformBuffer[i], &m_vpUniformBufferMemory[i]);

        //createBuffer(m_mainDevice.physicalDevice, m_mainDevice.logicalDevice, modelBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        //    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &m_modelDynUniformBuffer[i], &m_modelDynUniformBufferMemory[i]);
    }
}
//------------------------------------------------------------------------------
void VulkanRenderer::createDescriptorPool()
{
    // CREATE UNIFORM DESCRIPTOR POOL
    // Type of descriptors + how many DESCRIPTORS, not Descriptor Sets (combined makes the pool size)
    // View-Projection Pool
    VkDescriptorPoolSize vpPoolSize = {};
    vpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vpPoolSize.descriptorCount = static_cast<uint32_t>(m_vpUniformBuffer.size());

    // Model Pool (DYNAMIC)
    //VkDescriptorPoolSize modelPoolSize = {};
    //modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    //modelPoolSize.descriptorCount = static_cast<uint32_t>(m_modelDynUniformBuffer.size());

    // List of pool sizes
    std::vector<VkDescriptorPoolSize> descriptorPoolSizes = { vpPoolSize };

    // Data to create Descriptor Pool
    VkDescriptorPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.maxSets = static_cast<uint32_t>(m_swapchainImages.size());           // Maximum number of Descriptor Sets that can be created from pool
    poolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());   // Amount of Pool Sizes being passed
    poolCreateInfo.pPoolSizes = descriptorPoolSizes.data();                             // Pool Sizes to create pool with

    // Create Descriptor Pool
    VkResult result = vkCreateDescriptorPool(m_mainDevice.logicalDevice, &poolCreateInfo, nullptr, &m_descriptorPool);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Descriptor Pool!");
    }

    // CREATE SAMPLER DESCRIPTOR POOL
    // Texture sampler pool
    VkDescriptorPoolSize samplerPoolSize = {};
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = MAX_OBJECTS;

    VkDescriptorPoolCreateInfo samplerPoolCreateInfo = {};
    samplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    samplerPoolCreateInfo.maxSets = MAX_OBJECTS;
    samplerPoolCreateInfo.poolSizeCount = 1;
    samplerPoolCreateInfo.pPoolSizes = &samplerPoolSize;

    result = vkCreateDescriptorPool(m_mainDevice.logicalDevice, &samplerPoolCreateInfo, nullptr, &m_samplerDescriptorPool);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Descriptor Pool!");
    }
}
//------------------------------------------------------------------------------
void VulkanRenderer::createDescriptorSets()
{
    // Resize Descriptor Set list so one for every uniform buffer
    m_descriptorSets.resize(m_swapchainImages.size());

    // Create a vector with all the same m_descriptorSetLayout (that will be used for each element by descriptor sets)
    std::vector<VkDescriptorSetLayout> setLayouts(m_swapchainImages.size(), m_descriptorSetLayout);

    // Descriptor Set Allocation Info
    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = m_descriptorPool;                                     // Pool to allocate Descriptor Set from
    setAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_swapchainImages.size());  // Number of sets to allocate
    setAllocInfo.pSetLayouts = setLayouts.data();                                       // Layouts to use to allocate sets (1:1 relationship)

    // Allocate descriptor sets (multiple)
    VkResult result = vkAllocateDescriptorSets(m_mainDevice.logicalDevice, &setAllocInfo, m_descriptorSets.data());
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Descriptor Sets!");
    }

    // Update all of descriptor sets uniform buffer bindings
    assert(m_descriptorSets.size() == m_swapchainImages.size());
    for (size_t i = 0; i < m_vpUniformBuffer.size(); ++i)
    {
        // View-Projection DESCRIPTOR
        // Buffer info and data offset info
        VkDescriptorBufferInfo vpBufferInfo = {};
        vpBufferInfo.buffer = m_vpUniformBuffer[i];        // Buffer to get data from
        vpBufferInfo.offset = 0;                           // Position of start of data
        vpBufferInfo.range = sizeof(UboViewProjection);    // Size of data

        // Data about connection between binding and buffer
        VkWriteDescriptorSet vpSetWrite = {};
        vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vpSetWrite.dstSet = m_descriptorSets[i];                       // Descriptor Set to update
        vpSetWrite.dstBinding = 0;                                     // Binding to update (matches with binding on layout/shader)
        vpSetWrite.dstArrayElement = 0;                                // Index in array to update
        vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Type of descriptor
        vpSetWrite.descriptorCount = 1;                                // Amount of descriptor set to update
        vpSetWrite.pBufferInfo = &vpBufferInfo;                       // Information about buffer data to bind

        // Model DESCRIPTOR
        // Model Buffer info
        //VkDescriptorBufferInfo modelBufferInfo = {};
        //modelBufferInfo.buffer = m_modelDynUniformBuffer[i];    // Buffer to get data from
        //modelBufferInfo.offset = 0;                             // Position of start of data
        //modelBufferInfo.range = m_modelUniformAlignment;        // Size of data
        //
        //VkWriteDescriptorSet modelSetWrite = {};
        //modelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        //modelSetWrite.dstSet = m_descriptorSets[i];                                 // Descriptor Set to update
        //modelSetWrite.dstBinding = 1;                                               // Binding to update (matches with binding on layout/shader)
        //modelSetWrite.dstArrayElement = 0;                                          // Index in array to update
        //modelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;   // Type of descriptor
        //modelSetWrite.descriptorCount = 1;                                          // Amount of descriptor set to update
        //modelSetWrite.pBufferInfo = &modelBufferInfo;                               // Information about buffer data to bind

        // List of Descriptor Set Writes
        std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite };

        // Update the descriptor sets with new buffer/binding info
        vkUpdateDescriptorSets( m_mainDevice.logicalDevice,
                                static_cast<uint32_t>(setWrites.size()), setWrites.data(),
                                0, nullptr);
    }
}

//------------------------------------------------------------------------------
void VulkanRenderer::updateUniformBuffers(uint32_t imageIndex)
{
    // Copy View-Projection data (STATIC Uniform Buffer)
    void* data;
    vkMapMemory(m_mainDevice.logicalDevice, m_vpUniformBufferMemory[imageIndex], 0, sizeof(UboViewProjection), 0, &data);
    memcpy(data, &m_uboViewProjection, sizeof(UboViewProjection));
    vkUnmapMemory(m_mainDevice.logicalDevice, m_vpUniformBufferMemory[imageIndex]);

    /*/ Copy Model data (DYNAMIC Uniform Buffer)
    for (size_t i = 0; i < m_meshList.size(); ++i)
    {
        Model * thisModel = reinterpret_cast<Model*>( reinterpret_cast<uint64_t>(m_pModelTransferSpace) + (i * m_modelUniformAlignment) );
        *thisModel = m_meshList[i].getModel();
    }

    // Map the list of Model data
    vkMapMemory(m_mainDevice.logicalDevice, m_modelDynUniformBufferMemory[imageIndex], 0,
                m_modelUniformAlignment * m_meshList.size(), 0, &data);
    memcpy(data, m_pModelTransferSpace, m_modelUniformAlignment * m_meshList.size());
    vkUnmapMemory(m_mainDevice.logicalDevice, m_modelDynUniformBufferMemory[imageIndex]);
    /**/
}

//------------------------------------------------------------------------------
void VulkanRenderer::recordCommands(uint32_t currentImageIdx)
{
    // Information about how to begin each command buffer
    VkCommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;   // Buffer can be resubmitted when it has already been submitted and is awaiting execution

    // Information about how to begin a render pass (only needed for graphical applications)
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = m_renderPass;                          // Render Pass to begin
    renderPassBeginInfo.renderArea.offset = { 0, 0 };                       // Start point of render pass in pixels
    renderPassBeginInfo.renderArea.extent = m_swapChainExtent;              // Size of region to run render pass on (starting at offset)
        //{ 0.59f, 0.69f, 0.53f, 1.0f }     // Light green
        //{ 0.37f, 0.43f, 0.22f, 1.0f }     // Olive Green
        //{ 0.6f, 0.65f, 0.4f, 1.0f }       // Light olive green
        //{ 1.0f, 0.86, 0.72f, 1.0f }       // Light orange
        //{ 0.92f, 0.56, 0.24f, 1.0f }      // Orange
        std::array<VkClearValue, 2> clearValues = {};                       // Clear Color values
        clearValues[0].color = { 0.37f, 0.43f, 0.22f, 1.0f };               // Color clear RGBA (Red, Green, Blue, Alpha)
        clearValues[1].depthStencil.depth = 1.0f;                           // Depth clear value (1.0f is the farthest from camera)
    renderPassBeginInfo.pClearValues = clearValues.data();                  // Array of clear values
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());

    renderPassBeginInfo.framebuffer = m_swapChainFramebuffers[currentImageIdx];

    // Start recording commands to command buffer!
    VkResult result = vkBeginCommandBuffer(m_commandBuffers[currentImageIdx], &bufferBeginInfo);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to START recording a Command Buffer!");
    }

        // Begin Render Pass
        vkCmdBeginRenderPass(m_commandBuffers[currentImageIdx], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            // Bind Pipeline to be used in render pass
            vkCmdBindPipeline(m_commandBuffers[currentImageIdx], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

            // Loop Mesh list
            for (size_t meshIdx = 0; meshIdx < m_meshList.size(); ++meshIdx)
            {
                // Bind mesh Vertex buffers
                VkBuffer vertexBuffers[] = { m_meshList[meshIdx].getVertexBuffer() };   // Buffers to bind
                VkDeviceSize offsets[] = { 0 };                                         // Offsets into buffers being bound
                vkCmdBindVertexBuffers(m_commandBuffers[currentImageIdx], 0, 1, vertexBuffers, offsets);  // Command to bind vertex buffer before drawing with them

                // Bind mesh Index buffer (with 0 offset and using the uint32 type)
                vkCmdBindIndexBuffer(m_commandBuffers[currentImageIdx], m_meshList[meshIdx].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

                // Dynamic Uniform Buffer offset amount
                //uint32_t dynamicOffset = static_cast<uint32_t>(m_modelUniformAlignment * meshIdx);

                // Push constants to given shader stage directly (no buffer is used)
                Model model = m_meshList[meshIdx].getModel();
                vkCmdPushConstants(
                    m_commandBuffers[currentImageIdx],
                    m_pipelineLayout,
                    VK_SHADER_STAGE_VERTEX_BIT, // Shader stage where to push constants
                    0,                          // Offset of push constants to update
                    sizeof(Model),              // Size of data being pushed
                    &model);                    // Actual data being pushed (can be an array)

                // Group of Descriptor sets for the textures                    
                std::array<VkDescriptorSet, 2> descriptorSetGroup = { m_descriptorSets[currentImageIdx],
                    m_samplerDescriptorSets[m_meshList[meshIdx].getTextureIdx()] };
                
                // Bind Descriptor Sets
                vkCmdBindDescriptorSets(m_commandBuffers[currentImageIdx], VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout,
                    0, static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), 0, nullptr);

                // Execute pipeline
                vkCmdDrawIndexed(m_commandBuffers[currentImageIdx], m_meshList[meshIdx].getIndexCount(), 1, 0, 0, 0);
            }

        // End Render Pass
        vkCmdEndRenderPass(m_commandBuffers[currentImageIdx]);

    // Stop recording to command buffer
    result = vkEndCommandBuffer(m_commandBuffers[currentImageIdx]);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to STOP recording a Command Buffer!");
    }
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
    for (const auto &device : deviceList)
    {
        if (checkDeviceSuitable(device))
        {
            m_mainDevice.physicalDevice = device;
            break;
        }
    }

    // Get properties from our Physical device
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(m_mainDevice.physicalDevice, &deviceProperties);

    //m_minUniformBufferOffset = deviceProperties.limits.minUniformBufferOffsetAlignment;

    cout << "Highest Supported Vulkan Version (by Physical Device): " << getVersionString(deviceProperties.apiVersion) << endl;
}

//------------------------------------------------------------------------------
void VulkanRenderer::allocateDynamicBufferTransferSpace()
{
    /*/ Calculate alignment of model data
    m_modelUniformAlignment =   (sizeof(Model) + (m_minUniformBufferOffset - 1))
                                & ~(m_minUniformBufferOffset - 1);

    // Create space in memory to hold dynamic buffer that is aligned to our required alignment and holds MAX_OBJECTS
    m_pModelTransferSpace = (Model*)_aligned_malloc(m_modelUniformAlignment * MAX_OBJECTS, m_modelUniformAlignment);
    /**/
}

//------------------------------------------------------------------------------
bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>* extensionsToCheck)
{
    VkResult vkRes = VkResult::VK_SUCCESS;

    // Need to get the number of extensions to create an array of a matching size
    uint32_t extensionsCount = 0;
    vkRes = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
    cout << "Vulkan Instance Extensions (available): " << extensionsCount << endl;

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
        //cout << endl;
        //for (auto &extension : extensions)
        //{
        //    cout << "   Name: " << extension.extensionName << endl;
        //    cout << "Version: " << extension.specVersion << endl;
        //}
    }

    // Check if given extensions are within the availables
    for (const auto &checkExtension : *extensionsToCheck)
    {
        bool hasExtension = false;
        for (const auto &extension : extensions)
        {
            if (strcmp(checkExtension, extension.extensionName) == 0)
            {
                hasExtension = true;
                break;
            }
        }

        if (!hasExtension)
        {
            cout << "Required Vulkan Instance extension '" << checkExtension << "' is NOT supported." << endl;
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
    cout << "Vulkan Device Extensions (available): " << extensionsCount << endl;

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
    uint32_t validationLayerCount;
    vkEnumerateInstanceLayerProperties(&validationLayerCount, nullptr);

    // Check if no validation layers found AND we want at least 1 layer
    if (validationLayerCount == 0 && g_validationLayers.size() > 0)
    {
        return false;
    }

    std::vector<VkLayerProperties> availableLayers(validationLayerCount);
    vkEnumerateInstanceLayerProperties(&validationLayerCount, availableLayers.data());

    for (const char* layerName : g_validationLayers)
    {
        bool layerFound = false;

        for (const auto &layerProperties : availableLayers)
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
    /**/
    // Information about what the device can do (geom shaders, tessellation shaders, wide lines, etc.)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    QueueFamilyIndices indices = getQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);

    bool swapChainValid = false;
    if (extensionsSupported)
    {
        SwapchainDetails swapChainDetails = getSwapchainDetails(device);
        swapChainValid = !swapChainDetails.formats.empty() && !swapChainDetails.presentationModes.empty();
    }

    return indices.isValid() && extensionsSupported && swapChainValid
        && deviceFeatures.samplerAnisotropy;
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
    if (sg_validationEnabled)
    {
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
            indices.graphicsFamily = idx;   // If queue family is valid, then get index
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

        ++idx;
    }

    return indices;
}
//------------------------------------------------------------------------------
SwapchainDetails VulkanRenderer::getSwapchainDetails(VkPhysicalDevice device)
{
    SwapchainDetails swapChainDetails;

    // -- CAPABILITIES --
    // Get the Surface capabilities for the given surface on the given physical device
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &swapChainDetails.surfaceCapabilities);

    // -- SURFACE FORMATS --
    uint32_t formatsCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatsCount, nullptr);

    // If at least 1 format is supported, fill the structure
    if (formatsCount > 0)
    {
        swapChainDetails.formats.resize(formatsCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatsCount, swapChainDetails.formats.data());
    }

    // -- PRESENTATION MODES --
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
    for (const auto &format : formats)
    {
        if (    (format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
            &&  (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR))
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
    for (const auto &presentationMode : presentationModes)
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
VkFormat VulkanRenderer::chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
    // Loop through options and find a compatible one
    for (VkFormat format : formats)
    {
        // Get properties for the given format on this device
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(m_mainDevice.physicalDevice, format, &properties);

        // Depending on tiling choice, need to check for different bit flags
        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
        {
            return format;
        }
    }

    throw std::runtime_error("Failed to find a matching format!");
}
//------------------------------------------------------------------------------
VkImage VulkanRenderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                                    VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags,
                                    VkDeviceMemory* imageMemory)
{
    // CREATE IMAGE
    // Image Creation Info
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;                   // Type of image (1D, 2D, or 3D)
    imageCreateInfo.extent.width = width;                           // Width of image extent
    imageCreateInfo.extent.height = height;                         // Height of image extent
    imageCreateInfo.extent.depth = 1;                               // Depth of image (just 1, no 3D aspect)
    imageCreateInfo.mipLevels = 1;                                  // Number of mipmap levels
    imageCreateInfo.arrayLayers = 1;                                // Number of levels in image array
    imageCreateInfo.format = format;                                // Format type of image
    imageCreateInfo.tiling = tiling;                                // How image data should be "tiled" (arranged for optimal reading)
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;      // Layout of image data on creation
    imageCreateInfo.usage = useFlags;                               // Bit flags defining what image will be used for
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;                // Number of samples for multi-sampling
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;        // Whether image can be shared between queues

    // Create image
    VkImage image;
    VkResult result = vkCreateImage(m_mainDevice.logicalDevice, &imageCreateInfo, nullptr, &image);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create an Image!");
    }

    // CREATE MEMORY FOR IMAGE

    // Get memory requirements for a type of image
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(m_mainDevice.logicalDevice, image, &memoryRequirements);

    // Allocate memory using image requirements and user defined properties
    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memoryRequirements.size;
    memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(m_mainDevice.physicalDevice, memoryRequirements.memoryTypeBits, propFlags);

    result = vkAllocateMemory(m_mainDevice.logicalDevice, &memoryAllocInfo, nullptr, imageMemory);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate memory for image!");
    }

    // Connect (bind) memory to image
    vkBindImageMemory(m_mainDevice.logicalDevice, image, *imageMemory, 0);

    return image;
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
        throw std::runtime_error("Failed to create an Image View!");
    }
    return imageView;
}
//------------------------------------------------------------------------------
VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
{
    // Shader Module creation information
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = code.size();                                  // Size of code
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.data()); // Pointer to code (of uint32_t pointer type)

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(m_mainDevice.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a shader module!");
    }

    return shaderModule;
}

//------------------------------------------------------------------------------
int VulkanRenderer::createTexture(std::string fileName)
{
    // Create Texture Image and get its location in array
    int textureImageLoc = createTextureImage(fileName);

    // Create Image View and add to list
    VkImageView imageView = createImageView(m_textureImages[textureImageLoc], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    m_textureImageViews.push_back(imageView);

    // Create Texture Descriptor
    int descriptorLoc = createTextureDescriptor(imageView);

    // Return the location of the descriptor set with texture
    return descriptorLoc;
}

//------------------------------------------------------------------------------
int VulkanRenderer::createTextureImage(std::string fileName)
{
    // Load image file
    int width, height;
    VkDeviceSize imageSize;
    stbi_uc * imageData = loadTextureFile(fileName, &width, &height, &imageSize);

    // Create staging buffer to hold loaded data, ready to copy to device
    VkBuffer        imageStagingBuffer;
    VkDeviceMemory  imageStagingBufferMemory;
    createBuffer(m_mainDevice.physicalDevice, m_mainDevice.logicalDevice, imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &imageStagingBuffer, &imageStagingBufferMemory);

    // Copy image data to staging buffer
    void *data;
    vkMapMemory(m_mainDevice.logicalDevice, imageStagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, imageData, static_cast<size_t>(imageSize));
    vkUnmapMemory(m_mainDevice.logicalDevice, imageStagingBufferMemory);

    // Free original image data
    stbi_image_free(imageData);

    // Create the VkImage on the device to hold the final texture
    VkImage texImage;
    VkDeviceMemory texImageMemory;
    texImage = createImage(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texImageMemory);

    //--------------------------------------------
    // COPY DATA TO IMAGE
    // Transition image to be DST (DeSTination) for the copy operation
    transitionImageLayout(m_mainDevice.logicalDevice, m_graphicsQueue, m_graphicsCommandPool, 
        texImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy image data
    copyImageBuffer(m_mainDevice.logicalDevice, m_graphicsQueue, m_graphicsCommandPool, imageStagingBuffer, texImage, width, height);

    // Transition image to be shader readable for shader usage
    transitionImageLayout(m_mainDevice.logicalDevice, m_graphicsQueue, m_graphicsCommandPool,
        texImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Add texture data to vector for reference
    m_textureImages.push_back(texImage);
    m_textureImageMemory.push_back(texImageMemory);

    // Destroy staging buffers
    vkDestroyBuffer(m_mainDevice.logicalDevice, imageStagingBuffer, nullptr);
    vkFreeMemory(m_mainDevice.logicalDevice, imageStagingBufferMemory, nullptr);

    // Return an index of the new texture image
    return static_cast<int>(m_textureImages.size() - 1);
}

//------------------------------------------------------------------------------
int VulkanRenderer::createTextureDescriptor(VkImageView textureImageView)
{
    VkDescriptorSet descriptorSet;

    // Descriptor Set Allocation Info
    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = m_samplerDescriptorPool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &m_samplerSetLayout;

    // Allocate Descriptor Sets
    VkResult result = vkAllocateDescriptorSets(m_mainDevice.logicalDevice, &setAllocInfo, &descriptorSet);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Texture Descriptor Sets!");
    }

    // Texture Image Info
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;   // Image layout when in use
    imageInfo.imageView = textureImageView;                             // Image to bind to set
    imageInfo.sampler = m_textureSampler;                               // Sampler to use for set

    // Descriptor Write Info
    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;                            // Use the previously created imageInfo

    // Update new descriptor set
    vkUpdateDescriptorSets(m_mainDevice.logicalDevice, 1, &descriptorWrite, 0, nullptr);

    // Add descriptor set to list
    m_samplerDescriptorSets.push_back(descriptorSet);

    // Return descriptor set location
    return static_cast<int>(m_samplerDescriptorSets.size() - 1);
}

//------------------------------------------------------------------------------
stbi_uc* VulkanRenderer::loadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize)
{
    // Number of channels image uses
    int channels;

    // Load pixel data for image
    std::string fileLoc = "Textures/" + fileName;
    stbi_uc * image = stbi_load(fileLoc.c_str(), width, height, &channels, STBI_rgb_alpha);
    if (!image)
    {
        throw std::runtime_error("Failed to load a Texture file! ('Textures/" + fileName + "')");
    }

    // Calculate image size using given and known data
    *imageSize = (*width) * (*height) * STBI_rgb_alpha;

    return image;
}

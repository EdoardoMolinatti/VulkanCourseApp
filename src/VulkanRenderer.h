#pragma once

// Main graphics libraries (Vulkan API, GLFW [Graphics Library FrameWork])
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// C++ STL
#include <array>
#include <algorithm>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

// Project includes
#include "Mesh.h"
#include "Utilities.h"
#include "VulkanValidation.h"

// Disable warning about Vulkan unscoped enums for this entire file
#pragma warning( push )
#pragma warning(disable : 26812) // The enum type * is unscoped. Prefer 'enum class' over 'enum'.

class VulkanRenderer
{
public:
    VulkanRenderer();
    ~VulkanRenderer();

    // API
    int         init(GLFWwindow * newWindow);
    void        draw();
    void        cleanup();

private:
    // GLFW Components
    GLFWwindow *                    m_pWindow = nullptr;
    uint8_t                         m_currentFrame = 0U;    // Index of current frame. For Triple Buffer it'll be in {0, 1, 2}

    // Scene Objects
    std::vector<Mesh>               m_meshList;

    // Vulkan Components
    // - Main
    VkInstance                      m_pInstance = nullptr;
    VkDebugUtilsMessengerEXT        m_debugMessenger = 0U;
    struct {
        VkPhysicalDevice    physicalDevice = nullptr;
        VkDevice            logicalDevice = nullptr;
    }                               m_mainDevice;
    VkQueue                         m_graphicsQueue = nullptr;
    VkQueue                         m_presentationQueue = nullptr;
    VkSurfaceKHR                    m_surface = 0;      // '0' instead of 'nullptr' for compatibility with 32bit version
    VkSwapchainKHR                  m_swapChain = 0;    // '0' instead of 'nullptr' for compatibility with 32bit version

    std::vector<SwapchainImage>     m_swapchainImages;
    std::vector<VkFramebuffer>      m_swapChainFramebuffers;
    std::vector<VkCommandBuffer>    m_commandBuffers;

    // - Pipeline
    VkPipeline                      m_graphicsPipeline;
    VkPipelineLayout                m_pipelineLayout;
    VkRenderPass                    m_renderPass;

    // - Pools
    VkCommandPool                   m_graphicsCommandPool;

    // - Utility
    VkFormat                        m_swapChainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D                      m_swapChainExtent = {};

    // - Synchronisation
    std::vector<VkSemaphore>        m_imageAvailable;
    std::vector<VkSemaphore>        m_renderFinished;
    std::vector<VkFence>            m_drawFences;

    // Vulkan Functions
    // - Create Functions
    void createInstance();
    void createDebugMessenger();
    void createLogicalDevice();
    void createSurface();
    void createSwapchain();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSynchronisation();

    // - Record Functions
    void recordCommands();

    // - Get Functions
    void getPhysicalDevice();

    // - Support Functions
    // -- Checker Functions
    bool checkInstanceExtensionSupport(std::vector<const char*> * extensionsToCheck);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool checkValidationLayerSupport();
    bool checkDeviceSuitable(VkPhysicalDevice device);

    // -- Getter Functions
    std::vector<const char*>    getRequiredInstanceExtensions();
    QueueFamilyIndices          getQueueFamilies(VkPhysicalDevice device);
    SwapchainDetails            getSwapchainDetails(VkPhysicalDevice device);

    // -- Choose Functions
    VkSurfaceFormatKHR          chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR            chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
    VkExtent2D                  chooseBestSwapExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);

    // -- Create Functions
    VkImageView                 createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkShaderModule              createShaderModule(const std::vector<char> &code);
};

#pragma warning( pop )

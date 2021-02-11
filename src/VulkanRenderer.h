#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// C++ STL
#include <algorithm>
#include <set>
#include <stdexcept>
#include <vector>

#include "VulkanValidation.h"
#include "Utilities.h"


class VulkanRenderer
{
public:
    VulkanRenderer();
    ~VulkanRenderer();

    // API
    int         init(GLFWwindow * newWindow);
    void        cleanup();

private:
    GLFWwindow*                 m_pWindow = nullptr;

    // Vulkan members
    VkInstance                  m_pInstance = nullptr;
    VkDebugUtilsMessengerEXT    m_debugMessenger = 0U;
    struct {
        VkPhysicalDevice    physicalDevice = nullptr;
        VkDevice            logicalDevice = nullptr;
    }                           m_mainDevice;
    VkQueue                     m_graphicsQueue = nullptr;
    VkQueue                     m_presentationQueue = nullptr;
    VkSurfaceKHR                m_surface = nullptr;
    VkSwapchainKHR              m_swapChain = nullptr;
    std::vector<SwapchainImage> m_swapchainImages;

    // Disable warning about Vulkan unscoped enums for this entire file
#pragma warning( push )
#pragma warning(disable : 26812) // The enum type * is unscoped. Prefer 'enum class' over 'enum'.
    VkFormat                    m_swapChainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D                  m_swapChainExtent = {};
#pragma warning( pop )

    // Vulkan functions
    void createInstance();
    void createLogicalDevice();
    void setupDebugMessenger();
    void createSurface();
    void createSwapChain();

    // Get Functions
    void getPhysicalDevice();

    // Support functions
    bool checkInstanceExtensionSupport(std::vector<const char*> * extensionsToCheck);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    bool checkValidationLayerSupport();
    bool checkDeviceSuitable(VkPhysicalDevice device);

    // Utility functions
    QueueFamilyIndices          getQueueFamilies(VkPhysicalDevice device);
    std::vector<const char*>    getRequiredInstanceExtensions();
    SwapChainDetails            getSwapChainDetails(VkPhysicalDevice device);

    VkSurfaceFormatKHR          chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR            chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
    VkExtent2D                  chooseBestSwapExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);

    VkImageView                 createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
};


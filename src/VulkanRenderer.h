#ifndef VULKAN_RENDERER_H
#define VULKAN_RENDERER_H

// Main graphics libraries (Vulkan API, GLFW [Graphics Library FrameWork])
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// OpenGL Mathematics
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// C++ STL
#include <algorithm>
#include <array>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

// Project includes
#include "Mesh.h"
#include "Utilities.h"
#include "VulkanValidation.h"

using namespace Utilities;
// Utilities::SwapchainImage,   Utilities::SwapchainDetails, Utilities::QueueFamilyIndices,
// Utilities::MAX_FRAME_DRAWS,  Utilities::deviceExtensions, Utilities::createBuffer,
// Utilities::getVersionString, Utilities::readBinaryFile;

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
    bool        isWindowIconified();
    
    bool        updateModel(uint32_t modelId, glm::mat4 newModel);

    void        draw();
    void        cleanup();

private:
    // GLFW Components
    GLFWwindow *                    m_pWindow = nullptr;
    uint8_t                         m_currentFrame = 0U;    // Index of current frame. For Triple Buffer it'll be in {0, 1, 2}

    // Scene Objects
    std::vector<Mesh>               m_meshList;

    // Scene Settings
    struct UboViewProjection
    {
        glm::mat4 projection;
        glm::mat4 view;
    }                               m_uboViewProjection;    // UniformBufferObject View-Projection matrices. ⚠ Reflect shader decl. order

    // Vulkan Components
    // - Main
    VkInstance                      m_pInstance = nullptr;
    VkDebugUtilsMessengerEXT        m_debugMessenger = 0U;
    struct VK_DEVICE
    {
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

    // - Descriptors
    VkDescriptorSetLayout           m_descriptorSetLayout;
    VkPushConstantRange             m_pushConstantRange;

    VkDescriptorPool                m_descriptorPool;
    std::vector<VkDescriptorSet>    m_descriptorSets;

    std::vector<VkBuffer>           m_vpUniformBuffer;
    std::vector<VkDeviceMemory>     m_vpUniformBufferMemory;

    //std::vector<VkBuffer>           m_modelDynUniformBuffer;
    //std::vector<VkDeviceMemory>     m_modelDynUniformBufferMemory;

    //VkDeviceSize                    m_minUniformBufferOffset;
    //size_t                          m_modelUniformAlignment;
    //Model *                         m_pModelTransferSpace;

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
    void createDescriptorSetLayout();
    void createPushConstantRange();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSynchronisation();

    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();

    void updateUniformBuffers(uint32_t imageIndex);

    // - Record Functions
    void recordCommands(uint32_t imageIndex);

    // - Get Functions
    void getPhysicalDevice();

    // - Allocate Functions
    void allocateDynamicBufferTransferSpace();

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

#endif //VULKAN_RENDERER_H

#ifndef UTILITIES_H
#define UTILITIES_H

//----------------------------------------------------------------------------------------------------------------------
// Constants for C++ ISO Standards        (https://en.wikipedia.org/wiki/C%2B%2B for a list of standards)
constexpr auto __cpp_1998 = 199711L;    // C++98
constexpr auto __cpp_2003 = __cpp_1998; // C++03    (this was a TC - Technical Corrigendum of the standard, not a new release)
constexpr auto __cpp_2011 = 201103L;    // C++11
constexpr auto __cpp_2014 = 201402L;    // C++14
constexpr auto __cpp_2017 = 201703L;    // C++17
constexpr auto __cpp_2020 = 202002L;    // C++20
//constexpr auto __cpp_2023 = 202303L;    // C++23
// https://stackoverflow.com/questions/11053960/how-are-the-cplusplus-directive-defined-in-various-compilers#answer-11054055
// ⚠ Important Note: VisualStudio needs the string "/Zc:__cplusplus" to be defined in
// "Project Properties" -> "C/C++" -> "Command Line" ("Additional Options" pane)
// for All Configurations (Debug, Release, etc.) and all Architectures (x86, x64, etc.).
// https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus?view=msvc-160
//----------------------------------------------------------------------------------------------------------------------

// C++ STL
#include <iostream>
#include <fstream>
#if __cplusplus >= __cpp_2017
    #include <filesystem>           // Since C++17 - std::filesystem
#else
    #ifdef _WIN32
        #include <direct.h>
        #define GetCurrentDir _getcwd
    #else
        #include <unistd.h>
        #define GetCurrentDir getcwd
    #endif
    #include <stdio.h>
#endif
#include <string>
#include <vector>

// This is the first file included by "main", so we define here the STB implementation (for "stb_image.h")
#define STB_IMAGE_IMPLEMENTATION

// Disable warning about Vulkan unscoped enums for this entire file
#pragma warning( push )
#pragma warning(disable : 26812) // The enum type * is unscoped. Prefer 'enum class' over 'enum'.

// Main graphics libraries (GLFW [Graphics Library FrameWork] + Vulkan API)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// OpenGL Mathematics
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Using directives
using std::cout, std::endl;

// Constant expressions
constexpr uint32_t          WIN_DEFAULT_WIDTH = 1024;
constexpr uint32_t          WIN_DEFAULT_HEIGHT = 768;
#if (__cplusplus >= __cpp_2017)
#include <string_view>
constexpr std::string_view  WIN_DEFAULT_TITLE = "Test Window";
#else
constexpr char              WIN_DEFAULT_TITLE[] = "Test Window";
#endif


namespace Utilities
{
    //////////////////////////////
    // Constants
    //////////////////////////////
    const int MAX_FRAME_DRAWS = 3;
    //        MAX_FRAME_DRAWS should be less (or equal at max) to swapchain images
    const int MAX_OBJECTS = 2;

    //////////////////////////////
    // GLFW main Utilities
    //////////////////////////////
    // Callback for when the main window is minimized/restored
    // https://www.glfw.org/docs/3.3/window_guide.html#window_iconify
    static void glfwIconifiedWindowCbk(GLFWwindow* window, int iconified)
    {
        cout << "Window has been " << (iconified ? "iconified." : "restored.") << endl;
    }

    // Callback to manage GLFW errors
    // https://www.glfw.org/docs/3.3/quick.html#quick_capture_error
    static void glfwErrorCbk(int error_code, const char* description)
    {
        cout << "GLFW Error " << error_code << ": " << description << endl;
    }

    // Initialize Window
    static bool initWindow( GLFWwindow* &pWindow, std::string name = std::string(WIN_DEFAULT_TITLE),
                            const int width = WIN_DEFAULT_WIDTH, const int height = WIN_DEFAULT_HEIGHT)
    {
        // Register error callback before initialization
        glfwSetErrorCallback(glfwErrorCbk);

        // Initialization hints are set before glfwInit() and affect how the library behaves until termination.
        // https://www.glfw.org/docs/3.3/intro_guide.html#init_hints

        // Initialize GLFW
        if (!glfwInit())
        {
            cout << "ERROR: Can't initialize GLFW (OpenGL FrameWork)" << endl;
            return false;
        }

        // To use the Vulkan API we have to specify NO_API to GLFW library (to NOT work with OpenGL)
        // https://www.glfw.org/docs/3.3/window_guide.html#window_hints
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        pWindow = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
        if (nullptr == pWindow)
        {
            cout << "ERROR: Can't create the Main Window" << endl;
            return false;
        }

        // Register iconify callback
        glfwSetWindowIconifyCallback(pWindow, glfwIconifiedWindowCbk);

        // Make the window's context current (this only works for OpenGL contexts)
        //glfwMakeContextCurrent(pWindow);

        return true;
    }

    //////////////////////////////
    // Vulkan main Utilities
    //////////////////////////////
    // Device Extensions needed
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    // Vertex data representation
    struct Vertex
    {
        glm::vec3 pos; // Vertex Position (x, y, z)
        glm::vec3 col; // Vertex Colour (r, g, b)
    };

    // Indices (locations) of Queue Families (if they exist at all)
    struct QueueFamilyIndices
    {
        int graphicsFamily = -1;        // Location of Graphics Queue Family
        int presentationFamily = -1;    // Location of Presentation Queue Family
        int transferFamily = -1;        // N.B.: Vulkan guarantees that graphicsFamily also supports Transfer Queues

        // Check if queue families are valid
        bool isValid()
        {
            return (graphicsFamily >= 0 && presentationFamily >= 0);
        }
    };

    // Swap Chain detailed information
    struct SwapchainDetails
    {
        VkSurfaceCapabilitiesKHR        surfaceCapabilities = {};   // Surface properties (image, size, extent, etc.)
        std::vector<VkSurfaceFormatKHR> formats;                    // Surface image formats
        std::vector<VkPresentModeKHR>   presentationModes;          // Presentation mode (how images should be presented to the surface - i.e. screen)
    };

    // Swap Chain image
    struct SwapchainImage
    {
        VkImage     image;
        VkImageView imageView;
    };

    // Read binary file
    static std::vector<char> readBinaryFile(const std::string& filename)
    {
        // Open stream from given file
        // std::ios::binary tells stream to read file as binary
        // std::ios::ate tells stream to start reading At The End of file
        std::ifstream file(filename, std::ios::binary | std::ios::ate);

        // Check if file stream successfully opened
        if (!file.is_open())
        {
            std::string message("Failed to open file '" + filename + "'! Probably you didn't compile the shaders.");

            throw std::runtime_error(message.c_str());
        }

        // Get current read position and use to resize file buffer
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> fileBuffer(fileSize);

        // Move read position (seek to) the start of the file
        file.seekg(0);

        // Read the file data into the buffer (stream "fileSize" in total)
        file.read(fileBuffer.data(), fileSize);

        // Close stream
        file.close();

        return fileBuffer;
    }

    static uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties)
    {
        // Get properties of physical device memory
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
        {
            if (    (allowedTypes & (1 << i))                                                   // Index of memory type must match corresponding bit in allowedTypes
                &&  (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) // Desired property bit flags are part of memory type's property flags
            {
                // This memory type is valid, so return its index
                return i;
            }
        }

        return std::numeric_limits<uint32_t>::max();
    }

    static void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
        VkMemoryPropertyFlags bufferProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
    {
        // CREATE BUFFER (VERTEX/INDEX)
        // Information to create a buffer (doesn't include assigning memory)
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;                                // Size of buffer (size of 1 vertex * number of vertices)
        bufferInfo.usage = bufferUsage;                                // Multiple types of buffer possible
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;            // Similar to Swap Chain images, can share vertex buffers

        VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, buffer);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a Vertex Buffer!");
        }

        // GET BUFFER MEMORY REQUIREMENTS
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

        // ALLOCATE MEMORY TO BUFFER
        VkMemoryAllocateInfo memoryAllocInfo = {};
        memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memoryAllocInfo.allocationSize = memRequirements.size;
        memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice,   // Index of memory type on Physical Device that has required bit flags
            memRequirements.memoryTypeBits,                                     // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  : CPU can interact with memory
            bufferProperties);                                                  // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : Allows placement of data straight into buffer after mapping (otherwise would have to specify manually)

        // Allocate memory to VkDeviceMemory                                                                                                        
        result = vkAllocateMemory(device, &memoryAllocInfo, nullptr, bufferMemory);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate Buffer Memory!");
        }

        // Allocate memory to given vertex buffer
        vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
    }

    static VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool)
    {
        // Command buffer to hold transfer commands
        VkCommandBuffer commandBuffer;

        // Command Buffer details
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        // Allocate command buffer from pool
        vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

        // Information to begin the command buffer record
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;  // We're only using the command buffer once, so set up for one time submit

        vkBeginCommandBuffer(commandBuffer, &beginInfo);                // Begin recording transfer commands

        return commandBuffer;
    }

    static void endAndSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer)
    {
        // End commands
        vkEndCommandBuffer(commandBuffer);

        // Queue submission information
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        // Submit transfer command to transfer queue and wait until it finishes
        vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(queue);

        // Free temporary command buffer back to pool
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
    }

    static void copyBuffer( VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
                            VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
    {
        // Create buffer
        VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

        // Region of data to copy from and to
        VkBufferCopy bufferCopyRegion = {};
        bufferCopyRegion.srcOffset = 0;
        bufferCopyRegion.dstOffset = 0;
        bufferCopyRegion.size = bufferSize;

        // Command to copy src buffer to dst buffer
        vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

        endAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
    }

    static void copyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
                                VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height)
    {
        // Create buffer
        VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

        uint32_t regionCount = 1;
        VkBufferImageCopy imageRegion = {};
        imageRegion.bufferOffset = 0;                                           // Offset into data
        imageRegion.bufferRowLength = 0;                                        // Row length of data to calculate data spacing
        imageRegion.bufferImageHeight = 0;                                      // Image height to calculate data spacing
        imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;    // Which aspect of image to copy
        imageRegion.imageSubresource.mipLevel = 0;                              // Mipmap level to copy
        imageRegion.imageSubresource.baseArrayLayer = 0;                        // Starting array layer (if array)
        imageRegion.imageSubresource.layerCount = 1;                            // Number of layers to copy starting at baseArrayLayer
        imageRegion.imageOffset = { 0, 0, 0 };                                  // VkOffset3D into image (as opposed to raw data in bufferOffset)
        imageRegion.imageExtent = { width, height, 1 };                         // Size of region to copy as (x, y, z) values

        // Copy buffer to given image
        vkCmdCopyBufferToImage(transferCommandBuffer, srcBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regionCount, &imageRegion);

        endAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
    }

    static void transitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool commandPool,
                                      VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        // Create buffer
        VkCommandBuffer commandBuffer = beginCommandBuffer(device, commandPool);

        VkImageMemoryBarrier imageMemoryBarrier = {};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.oldLayout = oldLayout;                                   // Layout to transition from
        imageMemoryBarrier.newLayout = newLayout;                                   // Layout to transition to
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;           // Queue family to transition from
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;           // Queue family to transition to
        imageMemoryBarrier.image = image;                                           // Image being accessed and modified as part of barrier
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Aspect of image being altered
        imageMemoryBarrier.subresourceRange.baseMipLevel = 0;                       // First mip level to start alterations on
        imageMemoryBarrier.subresourceRange.levelCount = 1;                         // Number of mip levels to alter starting from baseMipLevel
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;                     // First layer to start alterations on
        imageMemoryBarrier.subresourceRange.layerCount = 1;                         // Number of layers to alter starting from baseArrayLayer

        VkPipelineStageFlags srcStage = 0U;
        VkPipelineStageFlags dstStage = 0U;

        // If transitioning from new image to image ready to receive data...
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_NONE;                  // Memory access stage transition must happen after...
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;    // Memory access stage transition must happen before...

            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        // If transitioning from transfer destination to shader readable...
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            srcStage, dstStage,     // Pipeline stages (match to src and dst AccessMasks)
            0U,                     // Dependency flags
            0U, nullptr,            // Memory Barrier count + data
            0U, nullptr,            // Buffer Memory Barrier count + data
            1U, &imageMemoryBarrier // Image Memory Barrier count + data
        );

        endAndSubmitCommandBuffer(device, commandPool, queue, commandBuffer);
    }

    static std::string getVersionString(uint32_t versionBitmask)
    {
        static const int MAX_STR_LENGTH = 64;
        char versionString[MAX_STR_LENGTH];

        uint32_t uMajorAPIVersion = VK_VERSION_MAJOR(versionBitmask);
        uint32_t uMinorAPIVersion = VK_VERSION_MINOR(versionBitmask);
        uint32_t uPatchAPIVersion = VK_VERSION_PATCH(versionBitmask);

        sprintf_s(versionString, MAX_STR_LENGTH, "%u.%u.%u", uMajorAPIVersion, uMinorAPIVersion, uPatchAPIVersion);

        return versionString;
    }

    ////////////////////
    // Generic Utilities
    ////////////////////

    // Macro to calculates the number of elements inside a C-style array (old fashioned)
    #define ARRAY_SIZE(a)   (sizeof(a) / sizeof(a[0]))

    // Current working directory
    static std::string getCurrentWorkingDirectory()
    {
#if __cplusplus >= __cpp_2017
        // https://en.cppreference.com/w/cpp/filesystem/current_path
        std::filesystem::path cwd = std::filesystem::current_path();
        return cwd.string();
#else
        char buff[FILENAME_MAX]; //create string buffer to hold path
        char* pRetVal = GetCurrentDir(buff, FILENAME_MAX);
        if (NULL == pRetVal)
        {
            cout << "Error getting the Current Working Directory path! Out of Memory?" << endl;
            return std::string("");
        }
        return std::string(buff);
#endif
    }

} //namespace VulkanAppUtils

#pragma warning( pop )

#endif //UTILITIES_H

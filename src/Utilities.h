#pragma once

// C++ STL
#include <fstream>

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Indices (locations) of Queue Families (if they exist at all)
struct QueueFamilyIndices {
    int graphicsFamily = -1;        // Location of Graphics Queue Family (main and necessary)
    int presentationFamily = -1;    // Location of Presentation Queue Family
    int transferFamily = -1;

    bool isValid()
    {
        return (graphicsFamily >= 0 && presentationFamily >= 0);
    }
};

// Swap Chain detailed information
struct SwapchainDetails {
    VkSurfaceCapabilitiesKHR        surfaceCapabilities = {};   // Surface properties (image, size, extent, etc.)
    std::vector<VkSurfaceFormatKHR> formats;                    // Surface image formats
    std::vector<VkPresentModeKHR>   presentationModes;          // Presentation mode (how images should be presented to the surface - i.e. screen)
};

// Swap Chain image
struct SwapchainImage {
    VkImage     image;
    VkImageView imageView;
};

// Read file utility function
static std::vector<char> readFile(const std::string &filename)
{
    // Open stream from given file
    // std::ios::binary tells stream to read file as binary
    // std::ios::ate tells stream to start reading from end of file
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    // Check if file stream successfully opened
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open a file!");
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

#pragma once

// Defines for C++ ISO Standards            (https://en.wikipedia.org/wiki/C%2B%2B for a list of standards)
#define __cpp_1997 199711L      // C++98
#define __cpp_2003 __cpp_1997   // C++98    (this was a TC - Technical Corrigendum - of the standard, not a new release)
#define __cpp_2011 201103L      // C++11
#define __cpp_2014 201402L      // C++14
#define __cpp_2017 201703L      // C++17
#define __cpp_2020 202002L      // C++20
// https://stackoverflow.com/questions/11053960/how-are-the-cplusplus-directive-defined-in-various-compilers#answer-11054055
// |!| Important Note: VisualStudio needs the string "/Zc:__cplusplus" to be defined in
// "Project Properties" -> "C/C++" -> "Command Line" ("Additional Options" pane) for all configurations (Debug, Release, etc.).
// https://docs.microsoft.com/en-us/cpp/build/reference/zc-cplusplus?view=msvc-160

// C++ STL
#include <fstream>
#if __cplusplus >= __cpp_2017
#include <filesystem>           // Since C++17
#endif
#include <string>
#include <vector>


////////////////////////
// Vulkan main Utilities
////////////////////////

// Device Extensions needed
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

// Read binary file
static std::vector<char> readFile(const std::string &filename)
{
    // Open stream from given file
    // std::ios::binary tells stream to read file as binary
    // std::ios::ate tells stream to start reading At The End of file
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    // Check if file stream successfully opened
    if (!file.is_open())
    {
        std::string message("Failed to open file '");
        message += filename + "'!";

        throw std::runtime_error(filename.c_str());
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

/////////////////////////////
// Generic Utilities (static)
/////////////////////////////

// Current working directory
static std::string getCurrentWorkingDirectory()
{
#if __cplusplus >= __cpp_2017
    // https://en.cppreference.com/w/cpp/filesystem/current_path
    std::filesystem::path cwd = std::filesystem::current_path();
    return cwd.string();
#else
    // TODO: Use an alternative (N.B.: Boost filesystem must be compiled)
    return "";
#endif
}

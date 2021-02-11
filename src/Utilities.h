#pragma once

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Indices (locations) of Queue Families (if they exist)
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
struct SwapChainDetails {
    VkSurfaceCapabilitiesKHR        surfaceCapabilities = {};   // Surface properties (image, size, extent, etc.)
    std::vector<VkSurfaceFormatKHR> formats;                    // Surface image formats
    std::vector<VkPresentModeKHR>   presentationModes;          // Presentation mode (how images should be presented to the surface - i.e. screen)
};

// Swap Chain image
struct SwapchainImage {
    VkImage     image;
    VkImageView imageView;
};

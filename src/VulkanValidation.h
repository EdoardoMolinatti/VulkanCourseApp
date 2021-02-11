#pragma once

#include <vulkan/vulkan.h>

#include <vector>

// Globals used only for Validation Layers
// VK_LAYER_KHRONOS_validation = All standard validation layers (Khronos Group)
const std::vector<const char*> g_validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
#ifdef NDEBUG
const bool g_enableValidationLayers = false;
#else
const bool g_enableValidationLayers = true;
#endif
// Validation Layers default configuration file is "vk_layer_settings.txt", and it sould be placed
// in the application's current Wording Directory. In its comments you can find instructions.


// Static class (only static methods, not instantiable)
class VulkanValidation
{
public:
    // Utility methods
    static void                    populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    // Vulkan proxy methods
    static VkResult                createDebugUtilsMessengerEXT(
        VkInstance                                  instance,
        const VkDebugUtilsMessengerCreateInfoEXT*   pCreateInfo,
        const VkAllocationCallbacks*                pAllocator,
        VkDebugUtilsMessengerEXT*                   pDebugMessenger);
    static void                    destroyDebugUtilsMessengerEXT(
        VkInstance                      instance,
        VkDebugUtilsMessengerEXT        debugMessenger,
        const VkAllocationCallbacks*    pAllocator);

    // Vulkan Callbacks
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT             messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*                                       pUserData);

private:
    // Disallow creating an instance of this object
    VulkanValidation() = delete;
    ~VulkanValidation() = delete;
    // prevent copying
    VulkanValidation(const VulkanValidation&) = delete;
    VulkanValidation& operator=(const VulkanValidation&) = delete;
};

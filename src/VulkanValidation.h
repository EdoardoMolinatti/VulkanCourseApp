#pragma once

// Disable warning about Vulkan unscoped enums for this entire file
#pragma warning( push )
#pragma warning(disable : 26812) // The enum type * is unscoped. Prefer 'enum class' over 'enum'.

// C++ STL
#include <vector>
// C++ Boost
#include <boost/algorithm/string.hpp>

// Globals used only for Validation Layers
#ifdef NDEBUG
const bool g_validationEnabled = false;
#else
const bool g_validationEnabled = true;
#endif

// VK_LAYER_KHRONOS_validation = All standard validation layers (Khronos Group)
const std::vector<const char*> g_validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
// Validation Layers default configuration file is "vk_layer_settings.txt", and it sould be placed
// in the application's current Wording Directory. In its comments you can find instructions.


// Static class (only static methods, not instantiable)
class VulkanValidation
{
public:
    // Utility methods
    static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                                    |   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                                |   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
        createInfo.pUserData = nullptr; // Optional. Could be the pointer to the main app
    }

    // Vulkan proxy methods
    static VkResult createDebugUtilsMessengerEXT(
        VkInstance                                  instance,
        const VkDebugUtilsMessengerCreateInfoEXT*   pCreateInfo,
        const VkAllocationCallbacks*                pAllocator,
        VkDebugUtilsMessengerEXT*                   pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }
        else {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    static void destroyDebugUtilsMessengerEXT(
        VkInstance                      instance,
        VkDebugUtilsMessengerEXT        debugMessenger,
        const VkAllocationCallbacks*    pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(instance, debugMessenger, pAllocator);
        }
    }

    // Vulkan Callbacks
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT             messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*                                       pUserData)
    {
        /*/
        messageSeverity VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
                    |   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        messageType     VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                      | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        struct VkDebugUtilsMessengerCallbackDataEXT {
            VkStructureType                             sType;
            const void*                                 pNext;
            VkDebugUtilsMessengerCallbackDataFlagsEXT   flags;
            const char*                                 pMessageIdName;
            int32_t                                     messageIdNumber;
            const char*                                 pMessage;
            uint32_t                                    queueLabelCount;
            const VkDebugUtilsLabelEXT*                 pQueueLabels;
            uint32_t                                    cmdBufLabelCount;
            const VkDebugUtilsLabelEXT*                 pCmdBufLabels;
            uint32_t                                    objectCount;
            const VkDebugUtilsObjectNameInfoEXT*        pObjects;
        };
        /**/
        std::string messageIdName = std::string(pCallbackData->pMessageIdName);
        std::string message = std::string(pCallbackData->pMessage);

        // ERROR
        if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            std::vector<std::string> tokens;
            boost::split(tokens, message, [](char sep) {return sep == '|'; });

            std::cerr << std::endl << "Message ID Name: '" << messageIdName << "'" << std::endl;
            if (tokens.size() == 3)
            {
                std::cerr << "[ERROR]" << tokens[2] << std::endl;
            }
            else {
                std::cerr << "[ERROR] " << message << std::endl;
            }
        }
        // WARNING
        else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            std::vector<std::string> tokens;
            boost::split(tokens, message, [](char sep) {return sep == '|'; });

            std::cerr << std::endl << "Message ID Name: '" << messageIdName << "'" << std::endl;
            if (tokens.size() == 3)
            {
                std::cerr << "[WARN ]" << tokens[2] << std::endl;
            }
            else {
                std::cerr << "[WARN ] " << message << std::endl;
            }
        }
        // INFO
        else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            std::cout << "[INFO ] " << message << std::endl;
        }
        // VERBOSE
        //else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        //    std::cout << "[VERBOSE] " << message << std::endl;
        //}

        return VK_FALSE;
        // The callback returns a boolean that indicates if the Vulkan call that triggered the validation layer message should be aborted.
        // If the callback returns VK_TRUE, then the call is aborted with the VK_ERROR_VALIDATION_FAILED_EXT error.
        // This is normally only used to test the validation layers themselves, so you should always return VK_FALSE.
    }

private:
    // Disallow creating an instance of this object
    VulkanValidation() = delete;
    ~VulkanValidation() = delete;
    // prevent copying
    VulkanValidation(const VulkanValidation&) = delete;
    VulkanValidation& operator=(const VulkanValidation&) = delete;
};

/*/ String split() utility method - TODO: prefer Boost (remove this)
#include <sstream>
std::vector<std::string> split(const std::string& s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}
/**/

#pragma warning( pop )

// GLFW (Graphics Library FrameWork) - it also includes Vulkan API (since it's defined GLFW_INCLUDE_VULKAN)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// C++ STL
#include <iostream>
//#include <stdexcept>
#include <string>
#include <vector>

// Project includes
#include "VulkanRenderer.h"
#include "Utilities.h"

using std::cout;
using std::endl;


// Global variables
GLFWwindow* window = nullptr;
VulkanRenderer vulkanRenderer;

void initWindow(std::string name = "Test Window", const int width = 1024, const int height = 768)
{
    // Initialize GLFW
    glfwInit();

    // To use Vulkan API we have to specify NO_API to GLFW library (to NOT work with OpenGL)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);

    // Make the window's context current
    glfwMakeContextCurrent(window);
}

int main()
{
    // Initialize Main Window
    initWindow("Vulkan Test App", 1440, 900);

    //--------------------------------------------------------------------------
    //// Vulkan extensions check
    //uint32_t extensionsCount = 0;
    //VkResult vkRes = VkResult::VK_SUCCESS;
    //vkRes = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
    //cout << "Vulkan Extensions number: " << extensionsCount << endl;
    //if (extensionsCount > 0)
    //{
    //    std::vector<VkExtensionProperties> extensions(extensionsCount);
    //    extensions.reserve(extensionsCount);
    //    do
    //    {
    //        // populate extensions buffer
    //        VkResult vkRes = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, extensions.data());
    //    } while (vkRes == VK_INCOMPLETE);
    //    cout << endl;
    //    for (auto &extension : extensions) {
    //        cout << "   Name: " << extension.extensionName << endl;
    //        cout << "Version: " << extension.specVersion << endl;
    //    }
    //}

    // GLM check
    glm::mat4 testMatrix(1.0f);
    glm::vec4 testVector(1.0f);

    auto testResult = testMatrix * testVector;
    //--------------------------------------------------------------------------

    // C++ compiler version check
    cout << "C++ Compiler version: " << __cplusplus << endl;
    // Current Working Directory
    cout << "Current Working Directory: '" << getCurrentWorkingDirectory() << "'" << endl;
    //
    cout << endl;

    // Initialize Vulkan Renderer instance
    if (EXIT_FAILURE == vulkanRenderer.init(window))
    {
        return EXIT_FAILURE;
    }

    // 3D Model update variables
    float angle = 0.0f;
    float deltaTime = 0.0f;
    float lastTime = 0.0f;

    // Main loop until window closed
    while (!glfwWindowShouldClose(window))
    {
        /* Poll for and process events */
        glfwPollEvents();

        /* Update model */
        float now = glfwGetTime();
        deltaTime = now - lastTime;
        lastTime = now;

        angle += 10.0f * deltaTime;
        if (angle > 360.0f) { angle -= 360.0f; }

        vulkanRenderer.updateModel(glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)));
        /**/

        /* Vulkan Draw current frame */
        vulkanRenderer.draw();
    }

    vulkanRenderer.cleanup();

    // Destroy GLFW window and terminate (stop) GLFW
    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}

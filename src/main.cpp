// GLFW (Graphics Library FrameWork) - it also includes Vulkan API (since it's defined GLFW_INCLUDE_VULKAN)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// C++ STL
#include <iostream>
//#include <stdexcept>
#include <string>
#include <vector>

using std::cout, std::endl;

// Project includes
#include "VulkanRenderer.h"
#include "Utilities.h"

using namespace Utilities;

// Global variables
GLFWwindow*     sg_pWindow = nullptr;
VulkanRenderer  sg_vulkanRenderer;


// MAIN ------------------------------------------------------------------------
int main()
{
    // Initialize Main Window
    if (!initWindow(sg_pWindow, "Vulkan Test App", 1440, 900))
    {
        cout << "ERROR: Can't initialize the Main Window" << endl;
        return EXIT_FAILURE;
    }

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
    if (EXIT_FAILURE == sg_vulkanRenderer.init(sg_pWindow))
    {
        cout << "ERROR: Can't initialize the Vulkan Renderer" << endl;
        return EXIT_FAILURE;
    }

    // 3D Model update variables
    float angle = 0.0f;
    float deltaTime = 0.0f;
    float lastTime = 0.0f;

    // Main loop until sg_pWindow closed
    while (!glfwWindowShouldClose(sg_pWindow))
    {
        /* Poll for and process events */
        glfwPollEvents();

        //----------------------------------------------------------------------
        /* Update model */
        float now = static_cast<float>(glfwGetTime());
        deltaTime = now - lastTime;
        lastTime = now;

        angle += 10.0f * deltaTime;
        if (angle > 360.0f) { angle -= 360.0f; }

        sg_vulkanRenderer.updateModel(glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)));
        //----------------------------------------------------------------------

        /* Vulkan Draw current frame */
        sg_vulkanRenderer.draw();
    }

    sg_vulkanRenderer.cleanup();

    // Destroy GLFW sg_pWindow and terminate (stop) GLFW
    glfwDestroyWindow(sg_pWindow);
    glfwTerminate();

    return EXIT_SUCCESS;
}

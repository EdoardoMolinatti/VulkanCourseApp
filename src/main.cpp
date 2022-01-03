// GLFW (Graphics Library FrameWork) - it also includes Vulkan API (since we define GLFW_INCLUDE_VULKAN)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// C++ STL
#include <iostream>
#include <stdexcept>
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

// Constant expressions (like #define). Set them to 1 to test basic features on your system
constexpr auto TEST_VULKAN_SDK  = 0;
constexpr auto TEST_GLM         = 0;


// MAIN ------------------------------------------------------------------------
int main()
{
    // C++ compiler version check
    cout << "C++ Compiler version: " << __cplusplus << " [YYYYMM] (C++11, C++14, C++17, C++20, C++23)" << endl;
    // Current Working Directory
    cout << "Current Working Directory: '" << getCurrentWorkingDirectory() << "'" << endl;
    //
    cout << endl;

    //--------------------------------------------------------------------------
    if (TEST_VULKAN_SDK == 1)
    {
        // Vulkan extensions check
        uint32_t extensionsCount = 0;
        VkResult vkRes = VkResult::VK_SUCCESS;
        vkRes = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
        cout << "Vulkan Extensions number: " << extensionsCount << endl;
        if (extensionsCount > 0)
        {
            std::vector<VkExtensionProperties> extensions(extensionsCount);
            extensions.reserve(extensionsCount);
            do
            {
                // populate extensions buffer
                VkResult vkRes = vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, extensions.data());
            } while (vkRes == VK_INCOMPLETE);
            for (auto& extension : extensions) {
                cout << "   Name: " << extension.extensionName << endl;
                cout << "Version: " << extension.specVersion << endl;
            }
        }
        else
        {
            cout << "Please install the Vulkan SDK on your system." << endl;
            return EXIT_FAILURE;
        }
        cout << endl;
    }
    //----------------------------------
    if (TEST_GLM == 1)
    {
        // GLM check
        glm::mat4 testMatrix(1.0f);
        glm::vec4 testVector(1.0f, 2.0f, 3.0f, 1.0f);

        auto testResult = testMatrix * testVector;

        char buffer[128] = {};
        sprintf_s(buffer, sizeof(buffer), "(%0.2f, %0.2f, %0.2f, %0.2f)", testResult.x, testResult.y, testResult.z, testResult.w);
        cout    << "- Test GLM -" //<< " - Result Type: " << typeid(testResult).name() << ", Result elements type: " << typeid(testResult.w).name()
                << "\nglm::mat4 * glm::vec4 == " << buffer
                << endl << endl;
        if (testVector != testResult)
        {
            cout << "Please install the GLM library (OpenGL Mathematics) on your system." << endl;
            // $(SolutionDir)../Libraries/GLM/
            return EXIT_FAILURE;
        }
    }
    //----------------------------------
    if (TEST_VULKAN_SDK || TEST_GLM)
    {
        cout << "All requested system TEST are done." << endl;
        system("pause");
        return EXIT_SUCCESS;
    }
    //--------------------------------------------------------------------------

    // Initialize Main Window
    if (!initWindow(sg_pWindow, "Vulkan Test App", 1440, 900))
    {
        cout << "ERROR: Can't initialize the Main Window" << endl;
        return EXIT_FAILURE;
    }

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
    // Frame number
    static unsigned long long frameNum = 0UL;

    // Main loop until window closed
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

        try
        {
            /* Vulkan Draw current frame */
            sg_vulkanRenderer.draw();

            ++frameNum;
        }
        catch (const std::exception& e)
        {
            cout << "RUNTIME ERROR: " << e.what() << endl;
            return EXIT_FAILURE;
        }
    }

    sg_vulkanRenderer.cleanup();

    // Destroy GLFW window and terminate (stop) GLFW
    glfwDestroyWindow(sg_pWindow);
    glfwTerminate();

    cout << endl << "Program END.\nRendered " << frameNum << " frames." << endl;
    return EXIT_SUCCESS;
}

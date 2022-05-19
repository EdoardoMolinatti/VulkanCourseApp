// GLFW (Graphics Library FrameWork) - it also includes Vulkan API (since we define GLFW_INCLUDE_VULKAN)
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// C++ STL
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using std::cout, std::endl;

// Project includes
#include "VulkanRenderer.h"
#include "Utilities.h"

// Global variables
GLFWwindow*     sg_pWindow = nullptr;
VulkanRenderer  sg_vulkanRenderer;

using namespace Utilities;

// Constant expressions (like #define). Set them to 1 to test basic features on your system
constexpr auto TEST_VULKAN_SDK  = 0;
constexpr auto TEST_GLM         = 0;


// MAIN ------------------------------------------------------------------------
int main()
{
    // C++ compiler version check (it's in _MSVC_LANG if "/Zc:__cplusplus" isn't defined in the project)
    cout << "C++ Compiler version: " << __cplusplus << " [YYYYMM]";
    switch (__cplusplus)
    {
    case __cpp_1998:
    //case __cpp_2003:
        cout << " (C++98/C++03)" << std::endl;
        break;
    case __cpp_2011:
        cout << " (C++11)" << std::endl;
        break;
    case __cpp_2014:
        cout << " (C++14)" << std::endl;
        break;
    case __cpp_2017:
        cout << " (C++17)" << std::endl;
        break;
    case __cpp_2020:
        cout << " (C++20)" << std::endl;
        break;
    //case __cpp_2023:
    //    cout << " (C++23)" << std::endl;
    //    break;
    default:
        cout << " (C++??)" << std::endl;
        break;
    }
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
            for (auto& extension : extensions)
            {
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

        if (sg_vulkanRenderer.isWindowIconified())
        {
            // Update the last time we got in this loop for the next frame
            lastTime = static_cast<float>(glfwGetTime());

            // Just sleep for 1 frame time if the window is iconified
            std::this_thread::sleep_for(std::chrono::milliseconds(17));
        }
        else
        {
            //----------------------------------------------------------------------
            /* Update the 3D Model */
            float now = static_cast<float>(glfwGetTime());
            deltaTime = now - lastTime;
            lastTime = now;

            angle += 10.0f * deltaTime;
            if (angle > 360.0f)
            {
                angle -= 360.0f;
            }

            //sg_vulkanRenderer.updateModel( glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)) );
            //------------------------------------
            glm::mat4 firstModel(1.0f);
            glm::mat4 secondModel(1.0f);
    
            firstModel = glm::translate(firstModel, glm::vec3(-2.0f, 0.0f, -5.0f));
            firstModel = glm::rotate(firstModel, glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f));
    
            secondModel = glm::translate(secondModel, glm::vec3(2.0f, 0.0f, -5.0f));
            secondModel = glm::rotate(secondModel, glm::radians(-angle * 10), glm::vec3(0.0f, 0.0f, 1.0f));
    
            sg_vulkanRenderer.updateModel(0, firstModel);
            sg_vulkanRenderer.updateModel(1, secondModel);
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
    }

    sg_vulkanRenderer.cleanup();

    // Destroy GLFW window and terminate (stop) GLFW
    glfwDestroyWindow(sg_pWindow);
    glfwTerminate();

    cout << endl << "Program END.\nRendered " << frameNum << " frames." << endl;
    return EXIT_SUCCESS;
}

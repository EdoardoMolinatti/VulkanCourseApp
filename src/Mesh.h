#ifndef MESH_H
#define MESH_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "Utilities.h"

using namespace Utilities;
// Utilities::Vertex, Utilities::createBuffer, Utilities::copyBuffer;


class Mesh
{
public:
    Mesh();
    Mesh(   VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, 
            VkQueue transferQueue, VkCommandPool transferCommandPool, 
            std::vector<Vertex> * vertices, std::vector<uint32_t> * indices);

    uint32_t    getVertexCount();
    VkBuffer    getVertexBuffer();

    uint32_t    getIndexCount();
    VkBuffer    getIndexBuffer();

    void        destroyBuffers();

    ~Mesh();

private:
    uint32_t            m_vertexCount = 0U;
    VkBuffer            m_vertexBuffer = 0;             // '0' instead of 'nullptr' for compatibility with 32bit version
    VkDeviceMemory      m_vertexBufferMemory = 0;       // '0' instead of 'nullptr' for compatibility with 32bit version

    uint32_t            m_indexCount = 0U;
    VkBuffer            m_indexBuffer = 0;              // '0' instead of 'nullptr' for compatibility with 32bit version
    VkDeviceMemory      m_indexBufferMemory = 0;        // '0' instead of 'nullptr' for compatibility with 32bit version

    VkPhysicalDevice    m_physicalDevice = nullptr;
    VkDevice            m_device= nullptr;              // This is our Logical Device

    // Methods
    void createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex> * vertices);
    void createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t> * indices);
};

#endif //MESH_H

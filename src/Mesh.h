#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "Utilities.h"

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
    VkBuffer            m_vertexBuffer = nullptr;
    VkDeviceMemory      m_vertexBufferMemory = nullptr;

    uint32_t            m_indexCount = 0U;;
    VkBuffer            m_indexBuffer = nullptr;
    VkDeviceMemory      m_indexBufferMemory = nullptr;

    VkPhysicalDevice    m_physicalDevice = nullptr;
    VkDevice            m_device= nullptr;              // This is our Logical Device

    // Methods
    void createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex> * vertices);
    void createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t> * indices);
};


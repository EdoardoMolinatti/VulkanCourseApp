#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "Utilities.h"

class Mesh
{
public:
    Mesh();
    Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, std::vector<Vertex> * vertices);

    uint32_t    getVertexCount();
    VkBuffer    getVertexBuffer();

    void        destroyVertexBuffer();

    ~Mesh();

private:
    uint32_t            m_vertexCount = 0U;
    VkBuffer            m_vertexBuffer = nullptr;
    VkDeviceMemory      m_vertexBufferMemory = nullptr;

    VkPhysicalDevice    m_physicalDevice = nullptr;
    VkDevice            m_device= nullptr;              // This is our Logical Device

    // Methods
    void        createVertexBuffer(std::vector<Vertex> * vertices);
    uint32_t    findMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags properties);
};


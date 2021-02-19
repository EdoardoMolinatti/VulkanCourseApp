#include "Mesh.h"

// Disable warning about Vulkan unscoped enums for this entire file
#pragma warning( push )
#pragma warning(disable : 26812) // The enum type * is unscoped. Prefer 'enum class' over 'enum'.

Mesh::Mesh()
{
}

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, std::vector<Vertex>* vertices)
{
    m_vertexCount = static_cast<int>(vertices->size());
    m_physicalDevice = newPhysicalDevice;
    m_device = newDevice;
    createVertexBuffer(vertices);
}

uint32_t Mesh::getVertexCount()
{
    return m_vertexCount;
}

VkBuffer Mesh::getVertexBuffer()
{
    return m_vertexBuffer;
}

void Mesh::destroyVertexBuffer()
{
    vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
    vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
}

Mesh::~Mesh()
{
}


// Private methods
void Mesh::createVertexBuffer(std::vector<Vertex>* vertices)
{
    // CREATE VERTEX BUFFER
    // Information to create a buffer (doesn't include assigning memory)
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(Vertex) * vertices->size();        // Size of buffer (size of 1 vertex * number of vertices)
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;       // Multiple types of buffer possible, we want Vertex Buffer
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;         // Similar to Swap Chain images, can share vertex buffers

    VkResult result = vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_vertexBuffer);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Vertex Buffer!");
    }

    // GET BUFFER MEMORY REQUIREMENTS
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, m_vertexBuffer, &memRequirements);

    // ALLOCATE MEMORY TO BUFFER (VkDeviceMemory)
    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memRequirements.size;
    memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirements.memoryTypeBits,   // Index of memory type on Physical Device that has required bit flags
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);        // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT  : CPU can interact with memory
                                                                                            // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : Allows placement of data straight into buffer after mapping (otherwise would have to specify manually)
    // Allocate memory to VkDeviceMemory
    result = vkAllocateMemory(m_device, &memoryAllocInfo, nullptr, &m_vertexBufferMemory);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Vertex Buffer Memory!");
    }

    // Allocate memory to given vertex buffer
    vkBindBufferMemory(m_device, m_vertexBuffer, m_vertexBufferMemory, 0);

    // MAP MEMORY TO VERTEX BUFFER
    void * data;                                                                // 1. Create pointer to a point in normal memory
    vkMapMemory(m_device, m_vertexBufferMemory, 0, bufferInfo.size, 0, &data);  // 2. "Map" the vertex buffer memory to that point
    memcpy(data, vertices->data(), (size_t)bufferInfo.size);                    // 3. Copy memory from vertices vector to the point
    vkUnmapMemory(m_device, m_vertexBufferMemory);                              // 4. Unmap the vertex buffer memory
}

uint32_t Mesh::findMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags properties)
{
    // Get properties of physical device memory
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if (    (allowedTypes & (1 << i))                                                   // Index of memory type must match corresponding bit in allowedTypes
            &&  (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) // Desired property bit flags are part of memory type's property flags
        {
            // This memory type is valid, so return its index
            return i;
        }
    }

    return -1;
}

#pragma warning( pop )

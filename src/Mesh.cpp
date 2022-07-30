#include "Mesh.h"


Mesh::Mesh()
{
}

Mesh::Mesh( VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, 
            VkQueue transferQueue, VkCommandPool transferCommandPool, 
            std::vector<Vertex>* vertices, std::vector<uint32_t> * indices,
            int textureIdx)
{
    m_vertexCount = static_cast<uint32_t>(vertices->size());
    m_indexCount = static_cast<uint32_t>(indices->size());
    m_physicalDevice = newPhysicalDevice;
    m_device = newDevice;
    createVertexBuffer(transferQueue, transferCommandPool, vertices);
    createIndexBuffer(transferQueue, transferCommandPool, indices);

    m_model.model = glm::mat4(1.0f);
    m_textureIdx = textureIdx;
}

Model Mesh::getModel()
{
    return m_model;
}

void Mesh::setModel(glm::mat4 newModel)
{
    m_model.model = newModel;
}

int Mesh::getTextureIdx()
{
    return m_textureIdx;
}

uint32_t Mesh::getVertexCount()
{
    return m_vertexCount;
}

VkBuffer Mesh::getVertexBuffer()
{
    return m_vertexBuffer;
}

uint32_t Mesh::getIndexCount()
{
    return m_indexCount;
}

VkBuffer Mesh::getIndexBuffer()
{
    return m_indexBuffer;
}

void Mesh::destroyBuffers()
{
    // Vertex Buffer Destroy + Free
    vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
    vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);
    // Index Buffer Destroy + Free
    vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
    vkFreeMemory(m_device, m_indexBufferMemory, nullptr);
}

Mesh::~Mesh()
{
}


// Private methods
void Mesh::createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices)
{
    // Get size of buffer needed for vertices
    VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

    // Temporary buffer to "stage" vertex data before transferring to GPU
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    // Create Staging Buffer and Allocate Memory to it
    createBuffer(m_physicalDevice, m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &stagingBuffer, &stagingBufferMemory);

    // MAP MEMORY TO VERTEX BUFFER (staging)
    void * data;                                                            // 1. Create pointer to a point in normal memory
    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);    // 2. "Map" the vertex buffer memory to that point
    memcpy(data, vertices->data(), (size_t)bufferSize);                     // 3. Copy memory from vertices vector to the point
    vkUnmapMemory(m_device, stagingBufferMemory);                           // 4. Unmap the vertex buffer memory

    // Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
    // Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU and only accessible by it and not CPU (host)
    createBuffer(m_physicalDevice, m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_vertexBuffer, &m_vertexBufferMemory);

    // Copy staging buffer to vertex buffer on GPU
    copyBuffer(m_device, transferQueue, transferCommandPool, stagingBuffer, m_vertexBuffer, bufferSize);

    // Destroy + Release Staging Buffer resources
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices)
{
    // Get size of buffer needed for indices
    VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();
    
    // Temporary buffer to "stage" index data before transferring to GPU
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(m_physicalDevice, m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

    // MAP MEMORY TO INDEX BUFFER (staging)
    void * data;                                                                
    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);            
    memcpy(data, indices->data(), (size_t)bufferSize);                            
    vkUnmapMemory(m_device, stagingBufferMemory);

    // Create buffer for INDEX data on GPU access only area
    createBuffer(m_physicalDevice, m_device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &m_indexBuffer, &m_indexBufferMemory);

    // Copy from staging buffer to GPU access buffer
    copyBuffer(m_device, transferQueue, transferCommandPool, stagingBuffer, m_indexBuffer, bufferSize);

    // Destroy + Release Staging Buffer resources
    vkDestroyBuffer(m_device, stagingBuffer, nullptr);
    vkFreeMemory(m_device, stagingBufferMemory, nullptr);
}

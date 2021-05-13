#include "Mesh.h"

#include <cstring>

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue,
           VkCommandPool transferCommandPool, std::vector<Vertex>* vertices, std::vector<uint32_t> *indices)

{
  vertexCount = vertices->size();
  indexCount = indices->size();
  physicalDevice = newPhysicalDevice;
  device = newDevice;
  createVertexBuffer(transferQueue, transferCommandPool, vertices);
  createIndexBuffer(transferQueue, transferCommandPool, indices);

    model.model = glm::mat4(1.0f);
}


void Mesh::createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex> *vertices)
{
  VkDeviceSize bufferSize = sizeof(Vertex)*vertices->size();

  // create staging buffer 
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
    &stagingBuffer, &stagingBufferMemory
  );

  void *data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, vertices->data(), (size_t) bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  createBuffer(physicalDevice, device, bufferSize, 
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    &vertexBuffer, &vertexBufferMemory
  );

  copyBuffer(device, transferQueue, transferCommandPool, stagingBuffer, vertexBuffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}
  
void Mesh::createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t> *indices)
{
  VkDeviceSize bufferSize = sizeof(uint32_t)*indices->size();

  // create staging buffer 
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  createBuffer(physicalDevice, device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
    &stagingBuffer, &stagingBufferMemory
  );

  void *data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, indices->data(), (size_t) bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  createBuffer(physicalDevice, device, bufferSize, 
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    &indexBuffer, &indexBufferMemory
  );

  copyBuffer(device, transferQueue, transferCommandPool, stagingBuffer, indexBuffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);

}


void Mesh::destroyBuffers()
{
  vkDestroyBuffer(device, vertexBuffer, nullptr);
  vkDestroyBuffer(device, indexBuffer, nullptr);
  vkFreeMemory(device, vertexBufferMemory, nullptr);
  vkFreeMemory(device, indexBufferMemory, nullptr);
}

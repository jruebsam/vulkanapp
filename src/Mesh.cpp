#include "Mesh.h"

#include <cstring>

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue,
           VkCommandPool transferCommandPool, std::vector<Vertex>* vertices)
{
  vertexCount = vertices->size();
  physicalDevice = newPhysicalDevice;
  device = newDevice;
  createVertexBuffer(transferQueue, transferCommandPool, vertices);
}


int Mesh::getVertexCount(){
  return vertexCount;
}

VkBuffer Mesh::getVertexBuffer(){
  return vertexBuffer;
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



void Mesh::destroyVertexBuffer()
{
  vkDestroyBuffer(device, vertexBuffer, nullptr);
  vkFreeMemory(device, vertexBufferMemory, nullptr);
}

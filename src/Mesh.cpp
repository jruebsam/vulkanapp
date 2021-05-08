#include "Mesh.h"

#include <cstring>

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, std::vector<Vertex>* vertices)
{
  vertexCount = vertices->size();
  physicalDevice = newPhysicalDevice;
  device = newDevice;
  createVertexBuffer(vertices);
}


int Mesh::getVertexCount(){
  return vertexCount;
}

VkBuffer Mesh::getVertexBuffer(){
  return vertexBuffer;
}

void Mesh::createVertexBuffer(std::vector<Vertex> *vertices){
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = sizeof(Vertex) * vertices->size();
  bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer);
  if(result != VK_SUCCESS)
  {
    throw  std::runtime_error("Failed to create Vertex Buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);
  
  VkMemoryAllocateInfo memAllocInfo{};
  memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memAllocInfo.allocationSize = memRequirements.size;
  memAllocInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
 
  result = vkAllocateMemory(device, &memAllocInfo, nullptr, &vertexBufferMemory);
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate Memory");
  }

  vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

  //map memory to vertex buffer;
  void *data;
  vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
  memcpy(data, vertices->data(), (size_t) bufferInfo.size);
  vkUnmapMemory(device, vertexBufferMemory);

}

uint32_t Mesh::findMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags properties)
{
  VkPhysicalDeviceMemoryProperties memoryProperties{};
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

  for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
  {
    if((allowedTypes & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties))
    {
      return i;
    }
  }
  throw std::runtime_error("Failed to find suitable memory type!"); 
}



void Mesh::destroyVertexBuffer()
{
  vkDestroyBuffer(device, vertexBuffer, nullptr);
  vkFreeMemory(device, vertexBufferMemory, nullptr);
}

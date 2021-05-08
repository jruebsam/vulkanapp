#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include "Utilities.h"



class Mesh
{
public:
  Mesh() = default;
  Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, 
       std::vector<Vertex> *vertices, std::vector<uint32_t> *indices);

  int getVertexCount(){return vertexCount;}
  int getIndexCount(){return indexCount;}

  VkBuffer getVertexBuffer(){return vertexBuffer;}
  VkBuffer getIndexBuffer(){return indexBuffer;}


  void destroyBuffers();

  ~Mesh() = default;

private:
  int vertexCount;
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;

  int indexCount;
  VkBuffer indexBuffer;
  VkDeviceMemory indexBufferMemory;

  VkPhysicalDevice physicalDevice;
  VkDevice device;

  void createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex> *vertices);
  void createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t> *indices);
};


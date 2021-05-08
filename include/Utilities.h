#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <fstream>
#include <glm/glm.hpp>

const int MAX_FRAME_DRAWS = 2;

const std::vector<const char*> deviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct Vertex
{
  glm::vec3 pos; 
  glm::vec3 col;
};


struct QueueFamilyIndices {
  int graphicsFamily = -1;
  int presentationFamily = -1;

  bool isValid()
  {
    return graphicsFamily >= 0 && presentationFamily >=0;
  }
};

struct SwapChainDetails {
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentationModes;
};

struct SwapChainImage {
  VkImage image;
  VkImageView imageView;
};


static std::vector<char> readFile(const std::string &filename)
{
  std::ifstream file(filename, std::ios::binary | std::ios::ate);

  if(!file.is_open())
  {
    throw std::runtime_error("Failed to open a file!");
  }

  size_t fileSize = (size_t) file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();
  
  return buffer;
};


static uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties)
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


static void createBuffer
(
  VkPhysicalDevice physicalDevice,
  VkDevice device,
  VkDeviceSize bufferSize,
  VkBufferUsageFlags bufferUsage,
  VkMemoryPropertyFlags bufferProperties, 
  VkBuffer * buffer,
  VkDeviceMemory * bufferMemory
)
{
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = bufferSize;
  bufferInfo.usage = bufferUsage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, buffer);
  if(result != VK_SUCCESS)
  {
    throw  std::runtime_error("Failed to create Vertex Buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);
  
  VkMemoryAllocateInfo memAllocInfo{};
  memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memAllocInfo.allocationSize = memRequirements.size;
  memAllocInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits, bufferProperties);
 
  result = vkAllocateMemory(device, &memAllocInfo, nullptr, bufferMemory);
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate Memory");
  }

  vkBindBufferMemory(device, *buffer, *bufferMemory, 0);

}

static void copyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
                       VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
{
  VkCommandBuffer transferCommandBuffer;

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = transferCommandPool;
  allocInfo.commandBufferCount = 1;

  vkAllocateCommandBuffers(device, &allocInfo, &transferCommandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(transferCommandBuffer, &beginInfo);

    // define data region
    VkBufferCopy bufferCopyRegion{};
    bufferCopyRegion.srcOffset = 0;
    bufferCopyRegion.dstOffset = 0;
    bufferCopyRegion.size = bufferSize;

    vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

  vkEndCommandBuffer(transferCommandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &transferCommandBuffer;

  vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(transferQueue);

  vkFreeCommandBuffers(device, transferCommandPool, 1, &transferCommandBuffer);
} 


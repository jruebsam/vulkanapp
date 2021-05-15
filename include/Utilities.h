#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <fstream>
#include <glm/glm.hpp>

const int MAX_FRAME_DRAWS = 2;
const int MAX_OBJECTS = 2;

const std::vector<const char*> deviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct Vertex
{
  glm::vec3 pos; 
  glm::vec3 col;
  glm::vec2 tex;
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

static VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool)
{
  VkCommandBuffer commandBuffer;

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);
  return commandBuffer;
}

static void endAndSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer)
{
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

static void copyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
                       VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
{
  VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

  // define data region
  VkBufferCopy bufferCopyRegion{};
  bufferCopyRegion.srcOffset = 0;
  bufferCopyRegion.dstOffset = 0;
  bufferCopyRegion.size = bufferSize;

  vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);
  endAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
}

static void copyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
                            VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height)
{
  VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

  VkBufferImageCopy imageRegion{};
  imageRegion.bufferOffset = 0;
  imageRegion.bufferRowLength = 0;
  imageRegion.bufferImageHeight = 0;
  imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageRegion.imageSubresource.mipLevel = 0;
  imageRegion.imageSubresource.baseArrayLayer = 0;
  imageRegion.imageSubresource.layerCount = 1;
  imageRegion.imageOffset = {0, 0, 0};
  imageRegion.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(transferCommandBuffer, srcBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);
  endAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
}

static void transitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
  VkCommandBuffer commandBuffer = beginCommandBuffer(device, commandPool);

  VkImageMemoryBarrier imageMemoryBarrier{};
  imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  imageMemoryBarrier.oldLayout = oldLayout;
  imageMemoryBarrier.newLayout = newLayout;
  imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  imageMemoryBarrier.image = image;
  imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
  imageMemoryBarrier.subresourceRange.levelCount = 1;
  imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
  imageMemoryBarrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags srcStage;
  VkPipelineStageFlags dstStage;


  if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL){
    imageMemoryBarrier.srcAccessMask = 0;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
  {
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }

  vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
  endAndSubmitCommandBuffer(device, commandPool, queue, commandBuffer);
}

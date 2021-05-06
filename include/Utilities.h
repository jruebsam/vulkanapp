#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <fstream>

const int MAX_FRAME_DRAWS = 2;

const std::vector<const char*> deviceExtensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
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

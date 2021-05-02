#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>

#include "Utilities.h"

class VulkanRenderer
{
public:
  VulkanRenderer();

  int init(GLFWwindow * newWindow);
  void cleanup();

  ~VulkanRenderer();

private:
  GLFWwindow * window;

  VkInstance instance;
  struct {
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
  } mainDevice;

  VkQueue graphicsQueue;
  VkDebugUtilsMessengerEXT debugMessenger;

  // create functions
  void createInstance();
  void createLogicalDevice();
  void setupDebugMessenger();

  // get functions
  void getPhysicalDevice();
  QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);

  // helper functions
  void appendGLFWExtensions(std::vector<const char*> *extensionList);
  void appendValidationExtensions(std::vector<const char*> *extensionList);

  bool checkInstanceExtensionSupport(std::vector<const char*> *checkExtensions);
  bool checkDeviceSuitable(VkPhysicalDevice device);
};


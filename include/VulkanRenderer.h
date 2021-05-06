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
  void draw();
  void cleanup();

  ~VulkanRenderer();

private:
  GLFWwindow * window;

  VkInstance instance;
  struct {
    VkPhysicalDevice physicalDevice;
    VkDevice logicalDevice;
  } mainDevice;

  // main components
  VkQueue graphicsQueue;
  VkQueue presentationQueue;
  VkDebugUtilsMessengerEXT debugMessenger;
  VkSurfaceKHR surface;
  VkSwapchainKHR swapchain;

  std::vector<SwapChainImage> swapChainImages;
  std::vector<VkFramebuffer> swapChainFrameBuffers;
  std::vector<VkCommandBuffer> commandBuffers;

  //utility components
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;

  //pipeline
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  VkRenderPass renderPass;

  //pools
  VkCommandPool graphicsCommandPool;


  // create functions
  void createInstance();
  void setupDebugMessenger();
  void createLogicalDevice();
  void createSurface();
  void createSwapChain();
  void createRenderPass();
  void createGraphicsPipeline();
  void createFrameBuffers();
  void createCommandPool();
  void createCommandBuffers();
  
  void createSynchronization();
  // record functions
  void recordCommands();

  // sync objects
  VkSemaphore imageAvailable;
  VkSemaphore renderFinished;


  // get functions
  void getPhysicalDevice();
  QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
  SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);

  // helper functions
  void appendGLFWExtensions(std::vector<const char*> *extensionList);
  void appendValidationExtensions(std::vector<const char*> *extensionList);

  bool checkInstanceExtensionSupport(std::vector<const char*> *checkExtensions);
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  bool checkDeviceSuitable(VkPhysicalDevice device);

  VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
  VkShaderModule createShaderModule(const std::vector<char> &code);

  // choose functions
  VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);
  VkPresentModeKHR chooseBestPresentationModel(const std::vector<VkPresentModeKHR> &presentationModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);

};


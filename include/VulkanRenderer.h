#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>

#include "Mesh.h"
#include "Utilities.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


class VulkanRenderer
{
public:
  VulkanRenderer();

  int init(GLFWwindow * newWindow);

  void updateModel(int modelId, glm::mat4 newModel);
  void draw();
  void cleanup();

  ~VulkanRenderer();

private:
  GLFWwindow * window;
  int currentFrame{0};

  // scene objects
  std::vector<Mesh> meshList;
  
  struct UboViewProjection{
    glm::mat4 projection; 
    glm::mat4 view; 
    
  } uboViewProjection;


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

  VkImage depthBufferImage;
  VkDeviceMemory depthBufferImageMemory;
  VkImageView depthBufferImageView;
  VkFormat depthFormat;


  //utility components
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;

  // descriptors
  VkDescriptorSetLayout descriptorSetLayout;
  VkPushConstantRange pushConstantRange;

  std::vector<VkBuffer> vpUniformBuffer;
  std::vector<VkDeviceMemory> vpUniformBufferMemory;

  std::vector<VkBuffer> modelDUniformBuffer;
  std::vector<VkDeviceMemory> modelDUniformBufferMemory;

  std::vector<VkDescriptorSet> descriptorSets;


  //VkDeviceSize minUniformBufferOffset;
  //size_t modelUniformAlignment;
  //UboModel* modelTransferSpace;

  // pipeline
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  VkRenderPass renderPass;

  // pools
  VkCommandPool graphicsCommandPool;
  VkDescriptorPool descriptorPool;

  // create functions
  void createInstance();
  void setupDebugMessenger();
  void createLogicalDevice();
  void createSurface();
  void createSwapChain();
  void createRenderPass();
  void createDescriptorSetLayout();
  void createPushConstantRange();
  void createUniformBuffers();
  void createGraphicsPipeline();
  void createDepthBufferImage();
  void createFrameBuffers();
  void createCommandPool();
  void createCommandBuffers();
  void createSynchronization();
  void createDescriptorPool();
  void createDescriptorSets();

  void updateUniformBuffers(uint32_t imageIndex);
  // record functions
  void recordCommands(uint32_t imageIndex);

  // allocate functions
  void allocateDynamicBufferTransferSpace();

  // sync objects
  std::vector<VkSemaphore> imageAvailable;
  std::vector<VkSemaphore> renderFinished;
  std::vector<VkFence> drawFences;

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

  VkImage createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
                      VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory *imageMemory);

  VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
  VkShaderModule createShaderModule(const std::vector<char> &code);

  // choose functions
  VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats);
  VkPresentModeKHR chooseBestPresentationModel(const std::vector<VkPresentModeKHR> &presentationModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities);
  VkFormat chooseSupportedFormat(const std::vector<VkFormat> &formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

};


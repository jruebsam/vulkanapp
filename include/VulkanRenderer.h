#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>

#include "Mesh.h"
#include "MeshModel.h"
#include "Utilities.h"
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


class VulkanRenderer
{
public:
  VulkanRenderer();

  int init(GLFWwindow * newWindow);
  int createMeshModel(std::string modelFile);

  void updateModel(int modelId, glm::mat4 newModel);
  void draw();
  void cleanup();

  ~VulkanRenderer();

private:
  GLFWwindow * window;
  int currentFrame{0};
  std::vector<MeshModel> modelList;

  // scene objects
  
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
  VkDescriptorSetLayout samplerSetLayout;

  VkPushConstantRange pushConstantRange;

  std::vector<VkBuffer> vpUniformBuffer;
  std::vector<VkDeviceMemory> vpUniformBufferMemory;

  std::vector<VkBuffer> modelDUniformBuffer;
  std::vector<VkDeviceMemory> modelDUniformBufferMemory;

  std::vector<VkDescriptorSet> descriptorSets;
  std::vector<VkDescriptorSet> samplerDescriptorSets;

  // assets 
  VkSampler textureSampler;
  std::vector<VkImage> textureImages;
  std::vector<VkDeviceMemory> textureImageMemory;
  std::vector<VkImageView> textureImageViews;


  // pipeline
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;
  VkRenderPass renderPass;

  // pools
  VkCommandPool graphicsCommandPool;
  VkDescriptorPool descriptorPool;
  VkDescriptorPool samplerDescriptorPool;

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

  int createTextureImage(std::string fileName);
  int createTexture(std::string fileName);
  void createTextureSampler();
  int createTextureDescriptor(VkImageView textureImage);


  void updateUniformBuffers(uint32_t imageIndex);
  // record functions
  void recordCommands(uint32_t imageIndex);

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

  // loader functions
  stbi_uc* loadTextureFile(std::string fileName, int *width, int *height, VkDeviceSize *imageSize);

};


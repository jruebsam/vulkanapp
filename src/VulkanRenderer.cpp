#include "VulkanRenderer.h"
#include "Validation.h"
#include "Utilities.h"
#include "Mesh.h"

#include <cstring>
#include <algorithm>
#include <set>
#include <array>

#include <stdlib.h>
 
#define _aligned_malloc(size, alignment) aligned_alloc(alignment, size)
#define _aligned_free(ptr) free(ptr)

//##############################( SETUP AND TEARDOWN )##############################

int VulkanRenderer::init(GLFWwindow * newWindow)
{
  window = newWindow;

  try {
    createInstance();
    setupDebugMessenger();
    createSurface();
    getPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createRenderPass();
    createDescriptorSetLayout();
    createPushConstantRange();

    createGraphicsPipeline();
    createFrameBuffers();
    createCommandPool();

    uboViewProjection.projection = glm::perspective(glm::radians(45.0f), (float) swapChainExtent.width/(float) swapChainExtent.height, 0.1f, 100.0f);
    uboViewProjection.projection[1][1] *= -1;
    uboViewProjection.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),  glm::vec3(0.0f, 1.0f, 0.0f));

    //create a mesh 
    std::vector<Vertex> meshVertices = {
      {{-0.4,  0.4, 0.0}, {1.0f, 0.0f, 0.0f}}, 
      {{-0.4, -0.4, 0.0}, {0.0f, 1.0f, 0.0f}}, 
      {{ 0.4, -0.4, 0.0}, {0.0f, 0.0f, 1.0f}}, 
      {{ 0.4,  0.4, 0.0}, {1.0f, 1.0f, 0.0f}}  
    };

    std::vector<Vertex> meshVertices2 = {
      {{-0.25,  0.6, 0.0}, {1.0f, 0.0f, 0.0f}}, 
      {{-0.25, -0.6, 0.0}, {0.0f, 1.0f, 0.0f}}, 
      {{ 0.25, -0.6, 0.0}, {0.0f, 0.0f, 1.0f}}, 
      {{ 0.25,  0.6, 0.0}, {1.0f, 1.0f, 0.0f}}  
    };

    std::vector<uint32_t>  meshIndices{0, 1, 2, 2, 3, 0};

    Mesh mesh1 = Mesh(mainDevice.physicalDevice, mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, &meshVertices, &meshIndices);
    Mesh mesh2 = Mesh(mainDevice.physicalDevice, mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, &meshVertices2, &meshIndices);
    meshList.push_back(mesh1);
    meshList.push_back(mesh2);

    createCommandBuffers();
    // allocateDynamicBufferTransferSpace();


    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createSynchronization();
  }
  catch (const std::runtime_error &e) {
    printf("ERROR: %s\n", e.what());
    return EXIT_FAILURE;
  }

  return 0;
}

void VulkanRenderer::updateModel(int modelId, glm::mat4 newModel)
{
  if(modelId >= meshList.size()) return;
  meshList[modelId].setModel(newModel);
}

void VulkanRenderer::draw()
{
  vkWaitForFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
  vkResetFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame]);


  uint32_t imageIndex;
  vkAcquireNextImageKHR(mainDevice.logicalDevice, swapchain, std::numeric_limits<uint64_t>::max(), imageAvailable[currentFrame], VK_NULL_HANDLE, &imageIndex);
  recordCommands(imageIndex);
  updateUniformBuffers(imageIndex);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &imageAvailable[currentFrame];

  VkPipelineStageFlags waitStages[] ={
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
  };
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &renderFinished[currentFrame];

  VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFences[currentFrame]);
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to submit draw operation to Graphics Queue");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &renderFinished[currentFrame];
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain;
  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(graphicsQueue, &presentInfo);
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to present Image!");
  }

  currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;; 
}
void VulkanRenderer::updateUniformBuffers(uint32_t imageIndex)
{
  void *data; 
  vkMapMemory(mainDevice.logicalDevice, vpUniformBufferMemory[imageIndex], 0, sizeof(UboViewProjection), 0, &data);
  memcpy(data, &uboViewProjection, sizeof(UboViewProjection));
  vkUnmapMemory(mainDevice.logicalDevice, vpUniformBufferMemory[imageIndex]);
  
  /*
  for(size_t i=0; i<meshList.size(); i++)
  {
    UboModel* thisModel = (UboModel *)((uint64_t) modelTransferSpace + (i*modelUniformAlignment));
    *thisModel = meshList[i].getModel();
  }

  vkMapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[imageIndex], 0, modelUniformAlignment*meshList.size(), 0, &data);
  memcpy(data, modelTransferSpace, modelUniformAlignment*meshList.size());
  vkUnmapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[imageIndex]);
  */
}

void VulkanRenderer::createSynchronization()
{
  imageAvailable.resize(MAX_FRAME_DRAWS);
  renderFinished.resize(MAX_FRAME_DRAWS);
  drawFences.resize(MAX_FRAME_DRAWS);

  VkSemaphoreCreateInfo semaphoreCreateInfo{};
  semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceCreateInfo{};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  

  for(size_t i=0; i<MAX_FRAME_DRAWS; i++){
    if(vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &imageAvailable[i]) != VK_SUCCESS||
       vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinished[i]) != VK_SUCCESS||
       vkCreateFence(mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &drawFences[i]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create Semaphore and/or Fence!");
    }
  }
}

void VulkanRenderer::setupDebugMessenger() {
    if (!enableValidationLayers) return;
    
    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}


void VulkanRenderer::cleanup()
{
  vkDeviceWaitIdle(mainDevice.logicalDevice);

  //_aligned_free(modelTransferSpace);
  
  vkDestroyDescriptorPool(mainDevice.logicalDevice, descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(mainDevice.logicalDevice, descriptorSetLayout, nullptr);

  for(size_t i=0; i<swapChainImages.size(); i++)
  {
    vkDestroyBuffer(mainDevice.logicalDevice, vpUniformBuffer[i], nullptr);
    vkFreeMemory(mainDevice.logicalDevice, vpUniformBufferMemory[i], nullptr);

    /*
    vkDestroyBuffer(mainDevice.logicalDevice, modelDUniformBuffer[i], nullptr);
    vkFreeMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[i], nullptr); */
  }

  for(size_t i=0; i<meshList.size(); i++){
    meshList[i].destroyBuffers();
  }
 
  for(size_t i=0; i<MAX_FRAME_DRAWS; i++){
    vkDestroySemaphore(mainDevice.logicalDevice, renderFinished[i], nullptr);
    vkDestroySemaphore(mainDevice.logicalDevice, imageAvailable[i], nullptr);
    vkDestroyFence(mainDevice.logicalDevice, drawFences[i], nullptr);
  }
  vkDestroyCommandPool(mainDevice.logicalDevice, graphicsCommandPool, nullptr);

  for(auto framebuffer: swapChainFrameBuffers)
  {
    vkDestroyFramebuffer(mainDevice.logicalDevice, framebuffer, nullptr);
  }

  vkDestroyPipeline(mainDevice.logicalDevice, graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(mainDevice.logicalDevice, pipelineLayout, nullptr);
  vkDestroyRenderPass(mainDevice.logicalDevice, renderPass, nullptr);

  for(auto image: swapChainImages)
  {
    vkDestroyImageView(mainDevice.logicalDevice, image.imageView, nullptr);
  }

  vkDestroySwapchainKHR(mainDevice.logicalDevice, swapchain, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  if (enableValidationLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
  }

  vkDestroyDevice(mainDevice.logicalDevice, nullptr);
  vkDestroyInstance(instance, nullptr);
}

VulkanRenderer::VulkanRenderer()
{
}

VulkanRenderer::~VulkanRenderer()
{
}

//##############################( CREATE INSTANCE )##############################


void VulkanRenderer::createInstance()
{
  if (enableValidationLayers && !checkValidationLayerSupport()) {
      throw std::runtime_error("Validation Layers requested, but not available!");
  }

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Vulkan App";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  std::vector<const char*> instanceExtensions = std::vector<const char*>();
  appendGLFWExtensions(&instanceExtensions);
  appendValidationExtensions(&instanceExtensions);

  if (!checkInstanceExtensionSupport(&instanceExtensions))
  {
    throw std::runtime_error("VkInstance does not support required extensions!");
  }

  createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
  createInfo.ppEnabledExtensionNames = instanceExtensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
  if (enableValidationLayers) {
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();

      populateDebugMessengerCreateInfo(debugCreateInfo);
      createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
  } else {
      createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
  }


  VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create a Vulkan Instance!");
  }
}

void VulkanRenderer::appendGLFWExtensions(std::vector<const char*> *extensionList)
{
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions;

  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  for (size_t i = 0; i < glfwExtensionCount; i++)
  {
    extensionList->push_back(glfwExtensions[i]);
  }
}

void VulkanRenderer::appendValidationExtensions(std::vector<const char*> *extensionList)
{
  if (enableValidationLayers) {
    extensionList->push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }
}

bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

  for (const auto &checkExtension : *checkExtensions)
  {
    bool hasExtension = false;
    for (const auto &extension : extensions)
    {
      if (strcmp(checkExtension, extension.extensionName) == 0)
      {
        hasExtension = true;
        break;
      }
    }

    if (!hasExtension)
    {
      return false;
    }
  }

  return true;
}

//##############################( GET PHYSICAL DEVICE )##############################

void VulkanRenderer::getPhysicalDevice()
{
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0)
  {
    throw std::runtime_error("Can't find GPUs that support Vulkan Instance!");
  }

  std::vector<VkPhysicalDevice> deviceList(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, deviceList.data());

  for (const auto &device : deviceList)
  {
    if (checkDeviceSuitable(device))
    {
      mainDevice.physicalDevice = device;
      break;
    }
  }
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(mainDevice.physicalDevice, &deviceProperties);
  //minUniformBufferOffset = deviceProperties.limits.minUniformBufferOffsetAlignment;
}

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
  // might be necessary to upgrade this check for more features
  QueueFamilyIndices indices = getQueueFamilies(device);

  bool extensionsSupported = checkDeviceExtensionSupport(device);

  bool swapChainValid = false;
  if(extensionsSupported)
  {
    SwapChainDetails swapChainDetails = getSwapChainDetails(device);
    swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
  }

  return indices.isValid() && extensionsSupported && swapChainValid;
}


bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
  if(extensionCount == 0) return false;

  std::vector<VkExtensionProperties> extensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

  for(const auto &deviceExtension: deviceExtensions)
  {
    bool hasExtension = false;
    for(const auto &extension: extensions)
    {
      if(strcmp(deviceExtension, extension.extensionName) == 0)
      {
        hasExtension = true;
        break;
      }
    }
    
    if(!hasExtension) return false;

  }

  return true;
}


QueueFamilyIndices VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
{
  QueueFamilyIndices indices;
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

  int i = 0;
  for (const auto &queueFamily : queueFamilyList)
  {
    if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      indices.graphicsFamily = i;
    }

    VkBool32 presentationSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
    if(queueFamily.queueCount > 0  && presentationSupport)
    {
      indices.presentationFamily = i;
    }


    if (indices.isValid())
    {
      break;
    }

    i++;
  }

  return indices;
}

SwapChainDetails VulkanRenderer::getSwapChainDetails(VkPhysicalDevice device)
{
  SwapChainDetails swapChainDetails;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

  if(formatCount != 0) {
    swapChainDetails.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.formats.data());
  }

  uint32_t presentationCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

  if(presentationCount != 0) {
    swapChainDetails.presentationModes.resize(presentationCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, swapChainDetails.presentationModes.data());
  } 

  return swapChainDetails;
}

//##############################( CREATE LOGICAL DEVICE )##############################

void VulkanRenderer::createLogicalDevice()
{
  QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

  // merge queues if they are the same 
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<int> queueFamilyIndices = {indices.graphicsFamily, indices.presentationFamily};

  for(int queueFamilyIndex: queueFamilyIndices){
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    queueCreateInfo.queueCount = 1;
    float priority = 1.0f;
    queueCreateInfo.pQueuePriorities = &priority;

    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
  deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
  deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();


  VkPhysicalDeviceFeatures deviceFeatures = {};
  deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

  VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);
  if (result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create a Logical Device!");
  }

  vkGetDeviceQueue(mainDevice.logicalDevice, indices.graphicsFamily, 0, &graphicsQueue);
  vkGetDeviceQueue(mainDevice.logicalDevice, indices.presentationFamily, 0, &presentationQueue);
}

//##############################( CREATE SURFACE )##############################

void VulkanRenderer::createSurface()
{
  VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);

  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create Surface!");
  }
}

//##############################( CREATE SWAPCHAIN )##############################

void VulkanRenderer::createSwapChain()
{
  SwapChainDetails swapChainDetails = getSwapChainDetails(mainDevice.physicalDevice);

  VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
  VkPresentModeKHR presentMode = chooseBestPresentationModel(swapChainDetails.presentationModes);
  VkExtent2D extent = chooseSwapExtent(swapChainDetails.surfaceCapabilities);

  uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;
  if(swapChainDetails.surfaceCapabilities.maxImageCount < imageCount) {
    imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR swapChainCreateInfo {};
  swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapChainCreateInfo.surface = surface;
  swapChainCreateInfo.imageFormat = surfaceFormat.format;
  swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
  swapChainCreateInfo.presentMode = presentMode;
  swapChainCreateInfo.imageExtent = extent;
  swapChainCreateInfo.minImageCount = imageCount; 
  swapChainCreateInfo.imageArrayLayers = 1;
  swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;
  swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapChainCreateInfo.clipped = VK_TRUE;

  QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice); 
  if(indices.graphicsFamily != indices.presentationFamily)
  {
    uint32_t queueFamilyIndices[] = {
      (uint32_t) indices.graphicsFamily,
      (uint32_t) indices.presentationFamily
    };

    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapChainCreateInfo.queueFamilyIndexCount = 2;
    swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
  }
  else
  {
    swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapChainCreateInfo.queueFamilyIndexCount = 0;
    swapChainCreateInfo.pQueueFamilyIndices = nullptr;
  }

  swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
  VkResult result = vkCreateSwapchainKHR(mainDevice.logicalDevice, &swapChainCreateInfo, nullptr, &swapchain);

  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create Swapchain!");
  }

  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;

  uint32_t swapChainImageCount;
  vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapChainImageCount, nullptr);

  std::vector<VkImage> images(swapChainImageCount);
  vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapChainImageCount, images.data());

  for(VkImage image: images)
  {
    SwapChainImage swapChainImage{};
    swapChainImage.image = image;
    swapChainImage.imageView = createImageView(image, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    
    swapChainImages.push_back(swapChainImage);
  }
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
  VkImageViewCreateInfo viewCreateInfo{};
  viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewCreateInfo.image = image;
  viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewCreateInfo.format = format;
  viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

  viewCreateInfo.subresourceRange.aspectMask = aspectFlags;
  viewCreateInfo.subresourceRange.baseMipLevel = 0;
  viewCreateInfo.subresourceRange.levelCount = 1;
  viewCreateInfo.subresourceRange.baseArrayLayer = 0;
  viewCreateInfo.subresourceRange.layerCount = 1;

  VkImageView imageView;
  VkResult result = vkCreateImageView(mainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create Image View!");
  }

  return imageView;
}

VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats)
{
  if(formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED){
    return {VK_FORMAT_R8G8B8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
  }

  for(const auto &format: formats) 
  {
    if((format.format == VK_FORMAT_R8G8B8_UNORM ||format.format == VK_FORMAT_B8G8R8_UNORM) &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return format;
    }
  }

  return formats[0];
}
  
VkPresentModeKHR VulkanRenderer::chooseBestPresentationModel(const std::vector<VkPresentModeKHR> &presentationModes)
{
  for(const auto &presentationMode: presentationModes)
  {
    if(presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      return presentationMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities)
{
  if(surfaceCapabilities.currentExtent.height != std::numeric_limits<uint32_t>::max())
  {
    return surfaceCapabilities.currentExtent;
  } 
  else 
  {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D newExtent{};
    newExtent.width = static_cast<uint32_t>(width);
    newExtent.height = static_cast<uint32_t>(height);

    newExtent.width  = std::max(surfaceCapabilities.minImageExtent.width,  std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
    newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

    return newExtent;
  }
}
//##############################( CREATE DESCRIPTOR LAYOUT )##############################

void VulkanRenderer::createDescriptorSetLayout()
{
  VkDescriptorSetLayoutBinding vpLayoutBinding{};
  vpLayoutBinding.binding = 0;
  vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  vpLayoutBinding.descriptorCount = 1;
  vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  vpLayoutBinding.pImmutableSamplers = nullptr;

  /*
  VkDescriptorSetLayoutBinding modelLayoutBinding{};
  modelLayoutBinding.binding = 1;
  modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  modelLayoutBinding.descriptorCount = 1;
  modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  modelLayoutBinding.pImmutableSamplers = nullptr;
  */

  std::vector<VkDescriptorSetLayoutBinding> layoutBindings = {vpLayoutBinding};//, modelLayoutBinding};

  VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
  layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
  layoutCreateInfo.pBindings = layoutBindings.data();

  VkResult result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &layoutCreateInfo, nullptr, &descriptorSetLayout);
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create DescriptorSet Layout!");
  }

}

void VulkanRenderer::createPushConstantRange()
{
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(Model);
}

void VulkanRenderer::createUniformBuffers()
{
  VkDeviceSize vpBufferSize = sizeof(UboViewProjection);
  //VkDeviceSize modelBufferSize = modelUniformAlignment*MAX_OBJECTS;

  vpUniformBuffer.resize(swapChainImages.size());
  vpUniformBufferMemory.resize(swapChainImages.size());
  //modelDUniformBuffer.resize(swapChainImages.size());
  //modelDUniformBufferMemory.resize(swapChainImages.size());

  for(size_t i=0; i<swapChainImages.size(); i++)
  {
    createBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice, vpBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,  &vpUniformBuffer[i], &vpUniformBufferMemory[i]);

    //createBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice, modelBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    //             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,  &modelDUniformBuffer[i], &modelDUniformBufferMemory[i]);
  }
}

void VulkanRenderer::createDescriptorPool()
{
  VkDescriptorPoolSize vpPoolSize{};
  vpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  vpPoolSize.descriptorCount = static_cast<uint32_t>(vpUniformBuffer.size());

  /*
  VkDescriptorPoolSize modelPoolSize{};
  modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  modelPoolSize.descriptorCount = static_cast<uint32_t>(modelDUniformBuffer.size());
  */

  std::vector<VkDescriptorPoolSize> poolSizes = {vpPoolSize};//, modelPoolSize};

  
  VkDescriptorPoolCreateInfo poolCreateInfo{};
  poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolCreateInfo.maxSets = static_cast<uint32_t>(vpUniformBuffer.size());
  poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolCreateInfo.pPoolSizes = poolSizes.data();

  VkResult result = vkCreateDescriptorPool(mainDevice.logicalDevice, &poolCreateInfo, nullptr, &descriptorPool);
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create Descriptor Pool!");
  }
}

void VulkanRenderer::createDescriptorSets()
{
  descriptorSets.resize(swapChainImages.size());
  std::vector<VkDescriptorSetLayout> setLayouts(swapChainImages.size(), descriptorSetLayout);

  VkDescriptorSetAllocateInfo setAllocInfo{};
  setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  setAllocInfo.descriptorPool = descriptorPool;
  setAllocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
  setAllocInfo.pSetLayouts = setLayouts.data();

  VkResult result = vkAllocateDescriptorSets(mainDevice.logicalDevice, &setAllocInfo, descriptorSets.data());
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create DescriptorSets!");
  }
  
  for(size_t i=0; i<swapChainImages.size(); i++)
  {

    /// VIEW PROJECTION
    VkDescriptorBufferInfo vpBufferInfo{};
    vpBufferInfo.buffer = vpUniformBuffer[i];
    vpBufferInfo.offset = 0;
    vpBufferInfo.range = sizeof(UboViewProjection);

    VkWriteDescriptorSet vpSetWrite{};
    vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vpSetWrite.dstSet = descriptorSets[i];
    vpSetWrite.dstBinding = 0;
    vpSetWrite.dstArrayElement = 0;
    vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vpSetWrite.descriptorCount = 1;
    vpSetWrite.pBufferInfo = &vpBufferInfo;


    /// MODEL
    /*
    VkDescriptorBufferInfo modelBufferInfo{};
    modelBufferInfo.buffer = modelDUniformBuffer[i];
    modelBufferInfo.offset = 0;
    modelBufferInfo.range = modelUniformAlignment;

    VkWriteDescriptorSet modelSetWrite{};
    modelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;;
    modelSetWrite.dstSet = descriptorSets[i];
    modelSetWrite.dstBinding = 1;
    modelSetWrite.dstArrayElement = 0;
    modelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelSetWrite.descriptorCount = 1;
    modelSetWrite.pBufferInfo = &modelBufferInfo;
    */

    std::vector<VkWriteDescriptorSet> setWrites = {vpSetWrite};//, modelSetWrite};

    vkUpdateDescriptorSets(mainDevice.logicalDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
  }
}

void VulkanRenderer::allocateDynamicBufferTransferSpace()
{
  /*
  modelUniformAlignment = (sizeof(UboModel) + minUniformBufferOffset -1) & ~(minUniformBufferOffset - 1);

  modelTransferSpace = (UboModel *) _aligned_malloc(modelUniformAlignment * MAX_OBJECTS, modelUniformAlignment);
  */

}

//##############################( CREATE GRAPHICS PIPELINE )##############################

void VulkanRenderer::createGraphicsPipeline()
{
  auto vertexShaderCode = readFile("shaders/vert.spv");
  auto fragmentShaderCode = readFile("shaders/frag.spv");

  // create shaders
  VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
  VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

  VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo{};
  vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertexShaderStageCreateInfo.module = vertexShaderModule;
  vertexShaderStageCreateInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo{};
  fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragmentShaderStageCreateInfo.module = fragmentShaderModule;
  fragmentShaderStageCreateInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo};
  
  // vertex data
  VkVertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(Vertex);
  bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  //position and color
  std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;

  attributeDescriptions[0].binding = 0;
  attributeDescriptions[0].location = 0;
  attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[0].offset = offsetof(Vertex, pos);

  attributeDescriptions[1].binding = 0;
  attributeDescriptions[1].location = 1;
  attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributeDescriptions[1].offset = offsetof(Vertex, col);

  
  //VERTEX INPUT
  VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
  vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
  vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  //INPUT ASSEMBLY
  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  //VIEWPORT + SCISSOR
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float) swapChainExtent.width;
  viewport.height = (float) swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  
  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapChainExtent;

  VkPipelineViewportStateCreateInfo viewPortStateCreateInfo{};
  viewPortStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewPortStateCreateInfo.viewportCount = 1;
  viewPortStateCreateInfo.pViewports = &viewport;
  viewPortStateCreateInfo.scissorCount = 1;
  viewPortStateCreateInfo.pScissors = & scissor;

  // --DYNAMIC STATES  TODO not used yet
  /*
  std::vector<VkDynamicState> dynamicStatesEnables;
  dynamicStatesEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
  dynamicStatesEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);

  VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
  dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicStateCreateInfo.dynamicStateCount =static_cast<uint32_t>(dynamicStatesEnables.size());
  dynamicStateCreateInfo.pDynamicStates = dynamicStatesEnables.data();
  */

  // RASTERIZER
  VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo{};
  rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizerCreateInfo.depthClampEnable = VK_FALSE;
  rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
  rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizerCreateInfo.lineWidth = 1.0f;
  rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

  // MULTISAMPLING
  VkPipelineMultisampleStateCreateInfo multiSamplingCreateInfo{};
  multiSamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multiSamplingCreateInfo.sampleShadingEnable = VK_FALSE;
  multiSamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  // BLENDING
  VkPipelineColorBlendAttachmentState colourState{};
  colourState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colourState.blendEnable = VK_TRUE; 
  colourState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colourState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colourState.colorBlendOp = VK_BLEND_OP_ADD;
  colourState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colourState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colourState.alphaBlendOp = VK_BLEND_OP_ADD;
  
  VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo{};
  colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
  colorBlendingCreateInfo.attachmentCount = 1;
  colorBlendingCreateInfo.pAttachments = &colourState;

  // PIPELINE LAYOUT (TODO: apply feature descriptor set layouts)
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
  pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
  pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
  pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

  VkResult result = vkCreatePipelineLayout(mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create Pipeline Layout!");
  }

  // TODO - DEPTH STENCIL TESTING
  
  VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineCreateInfo.stageCount = 2;
  pipelineCreateInfo.pStages = shaderStages;
  pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
  pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
  pipelineCreateInfo.pViewportState = &viewPortStateCreateInfo;
  pipelineCreateInfo.pDynamicState = nullptr;
  pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
  pipelineCreateInfo.pMultisampleState = &multiSamplingCreateInfo;
  pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
  pipelineCreateInfo.pDepthStencilState = nullptr;
  pipelineCreateInfo.layout = pipelineLayout;
  pipelineCreateInfo.renderPass = renderPass;
  pipelineCreateInfo.subpass = 0;

  // no pipeline derivatives
  pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineCreateInfo.basePipelineIndex = -1;

  result = vkCreateGraphicsPipelines(mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline);
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create Rendering Pipeline!"); 
  }


  // cleanup shaders after pipeline creation
  vkDestroyShaderModule(mainDevice.logicalDevice, fragmentShaderModule, nullptr);
  vkDestroyShaderModule(mainDevice.logicalDevice, vertexShaderModule, nullptr);
}

void VulkanRenderer::createRenderPass()
{
  // define color attachment
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; //before rendering
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; //after rendering

  // attachement reference
  VkAttachmentReference colorAttachmentReference{};
  colorAttachmentReference.attachment = 0;
  colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // create subpass description
  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentReference;

  // create subpass dependencies
  std::array<VkSubpassDependency, 2> subpassDependencies;
  //from image layout undefined to image layout color attachment optimal
  subpassDependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
  subpassDependencies[0].srcStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;

  subpassDependencies[0].dstSubpass    = 0;
  subpassDependencies[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  subpassDependencies[0].dependencyFlags = 0;

  //from 
  subpassDependencies[1].srcSubpass    = 0;
  subpassDependencies[1].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  subpassDependencies[1].dstSubpass    = VK_SUBPASS_EXTERNAL;
  subpassDependencies[1].dstStageMask  = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;

  subpassDependencies[1].dependencyFlags = 0;

  VkRenderPassCreateInfo renderPassCreateInfo{};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.attachmentCount = 1;
  renderPassCreateInfo.pAttachments = &colorAttachment;
  renderPassCreateInfo.subpassCount = 1;
  renderPassCreateInfo.pSubpasses = &subpass;
  renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
  renderPassCreateInfo.pDependencies = subpassDependencies.data();

  VkResult result = vkCreateRenderPass(mainDevice.logicalDevice,&renderPassCreateInfo, nullptr, &renderPass);
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create a Render Pass!");
  }


}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char> &code)
{
  VkShaderModuleCreateInfo shaderModuleCreateInfo{};
  shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shaderModuleCreateInfo.codeSize = code.size();
  shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule;
  VkResult result = vkCreateShaderModule(mainDevice.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create a Shader Module!");
  }

  return shaderModule;
}

//##############################( CREATE FRAMEBUFFERS )##############################
  
void VulkanRenderer::createFrameBuffers()
{
  swapChainFrameBuffers.resize(swapChainImages.size());

  for(size_t i=0; i<swapChainFrameBuffers.size(); i++)
  {
    std::array<VkImageView, 1> attachments = {
      swapChainImages[i].imageView
    };

    VkFramebufferCreateInfo frameBufferCreateInfo{};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.renderPass = renderPass;
    frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    frameBufferCreateInfo.pAttachments = attachments.data();
    frameBufferCreateInfo.width = swapChainExtent.width;
    frameBufferCreateInfo.height = swapChainExtent.height;
    frameBufferCreateInfo.layers = 1;

    VkResult result = vkCreateFramebuffer(mainDevice.logicalDevice, &frameBufferCreateInfo, nullptr,&swapChainFrameBuffers[i]);

    if(result != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Framebuffer!");
    }
  }
}
//##############################( CREATE COMMAND POOL AND BUFFERS )##############################
  
void VulkanRenderer::createCommandPool()
{
  QueueFamilyIndices indices = getQueueFamilies(mainDevice.physicalDevice);

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = indices.graphicsFamily;

  VkResult result = vkCreateCommandPool(mainDevice.logicalDevice, &poolInfo, nullptr, &graphicsCommandPool);
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to create Command Pool!");
  }
}

void VulkanRenderer::createCommandBuffers()
{
  commandBuffers.resize(swapChainFrameBuffers.size());
  VkCommandBufferAllocateInfo cbAllocInfo{};
  cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  cbAllocInfo.commandPool = graphicsCommandPool;
  cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

  cbAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  VkResult result = vkAllocateCommandBuffers(mainDevice.logicalDevice, &cbAllocInfo, commandBuffers.data());
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to allocate Command Buffers!");
  }
}

void VulkanRenderer::recordCommands(uint32_t imageIndex)
{
  //how to start a command buffer
  VkCommandBufferBeginInfo bufferBeginInfo{};
  bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  //how to start a render pass
  VkRenderPassBeginInfo renderPassBeginInfo{};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.renderPass = renderPass;
  renderPassBeginInfo.renderArea.offset = {0, 0};
  renderPassBeginInfo.renderArea.extent = swapChainExtent;

  VkClearValue clearValues[] ={
    {0.1f, 0.2f, 0.3f, 1.0f}
  };
  renderPassBeginInfo.pClearValues = clearValues;
  renderPassBeginInfo.clearValueCount = 1;

    //start record
    VkResult result = vkBeginCommandBuffer(commandBuffers[imageIndex], &bufferBeginInfo);
    if(result != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to record Command Buffer!");
    }

    //begin render pass
    renderPassBeginInfo.framebuffer = swapChainFrameBuffers[imageIndex];
    vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

      //bind and execute pipeline
      vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
      
      for(size_t j=0; j<meshList.size(); j++){
        VkBuffer vertexBuffers[] = {meshList[j].getVertexBuffer()};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffers[imageIndex], meshList[j].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

        //uint32_t dynamicOffset = static_cast<uint32_t>(modelUniformAlignment) * j;
        vkCmdPushConstants(commandBuffers[imageIndex], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Model), &meshList[j].getModel());

        vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[imageIndex], 0, nullptr);//1, &dynamicOffset);

        vkCmdDrawIndexed(commandBuffers[imageIndex], meshList[j].getIndexCount(), 1, 0, 0, 0);
      }



    //end render pass
    vkCmdEndRenderPass(commandBuffers[imageIndex]);

  //stop record
  result = vkEndCommandBuffer(commandBuffers[imageIndex]);
  if(result != VK_SUCCESS)
  {
    throw std::runtime_error("Failed to record Command Buffer!");
  }

}

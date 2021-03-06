#pragma once

#include <vector>
#include "glm/glm.hpp"
#include "Mesh.h"
#include <assimp/scene.h>

class MeshModel
{
public:
  MeshModel() = default;
  MeshModel(std::vector<Mesh> newMeshList);

  size_t getMeshCount();
  Mesh* getMesh(size_t index);

  glm::mat4 getModel();
  void setModel(glm::mat4 newModel);

  static std::vector<std::string> LoadMaterials(const aiScene* scene);

  static std::vector<Mesh> LoadNode(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, 
                                    VkQueue transferQueue, VkCommandPool transferCommandPool,
                                      aiNode *node, const aiScene *scene, std::vector<int> matToTex);

  static Mesh LoadMesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, 
                                    VkQueue transferQueue, VkCommandPool transferCommandPool,
                                      aiMesh *mesh, const aiScene *scene, std::vector<int> matToTex);

  void destroyMeshModel();
  ~MeshModel();

private:
  std::vector<Mesh> meshList;
  glm::mat4 model;

};

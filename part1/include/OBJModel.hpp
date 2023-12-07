// Guard to prevent multiple inclusions of the header file
#ifndef OBJMODEL_HPP
#define OBJMODEL_HPP

// Required dependencies and libraries
#include "Texture.hpp"
#include <fstream>
#include <glad/glad.h> // OpenGL loader library
#include <glm/glm.hpp> // GLM for matrix and vector operations
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

// Structure to represent the material properties of an object
struct Material {
  float ns;         // Specular exponent
  float ka[3];      // Ambient color
  float kd[3];      // Diffuse color
  float ks[3];      // Specular color
  float ke[3];      // Emissive color
  float ni;         // Optical density (index of refraction)
  float d;          // Dissolve (transparency)
  int illum;        // Illumination model
  Texture map_kd;   // Diffuse texture map
  Texture map_bump; // Bump map texture
  Texture map_ks;   // Specular texture map
};

// Class to represent an OBJ model
class OBJModel {
public:
  OBJModel();                            // Default constructor
  OBJModel(const std::string &filepath); // Constructor to load model from file
  ~OBJModel();                           // Destructor to clean up resources

  void render() const; // Render the model
  void
  loadModelFromFile(const std::string &filepath); // Load model data from file
  void SetShaderMaterialUniforms(
      GLuint shaderProgram);         // Set material properties in the shader
  void setCacheMode(const int mode); // Set cache mode to make indices order

private:
  // Vertex structure to represent a vertex with position, texture
  // coordinates, and normal vector
  struct Vertex {
    glm::vec3 position;  // Vertex position
    glm::vec2 texCoords; // Texture coordinates
    glm::vec3 normal;    // Normal vector
  };

  // Model data
  std::vector<Vertex> vertices; // List of vertices
  std::vector<GLuint> indices;  // Indices for indexed drawing
  std::unordered_map<std::string, Texture>
      texturesLoaded; // Map of loaded textures to avoid duplication

  // OpenGL buffer objects
  GLuint vao, vbo, ebo; // Vertex Array Object, Vertex Buffer Object, and

  std::vector<GLuint> oriIndices;
  std::vector<GLuint> optiIndices;

  Material material;   // Material properties of the model
  void setupBuffers(); // Setup the VAO, VBO, and EBO
  void
  LoadMaterials(const std::string
                    &mtlFilePath); // Load material properties from a .mtl file
  std::vector<glm::vec3>
  generateOffsetVectors(int maxOffset); // Generate vectors for replicating
  void optimizingIndices(); // Optimize the order of indices based on Foryth's
                            // algorithm
};

#endif // OBJMODEL_HPP

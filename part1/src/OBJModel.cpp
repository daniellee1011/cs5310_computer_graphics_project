#include "OBJModel.hpp"
#include <algorithm>
#include <chrono>
#include <random>
#include <sstream>

#define FORSYTH_IMPLEMENTATION
#include "forsyth.h"

// Default constructor
OBJModel::OBJModel() {
  std::cout << "OBJModel default constructor: Nothing loaded yet" << std::endl;
}

// Constructor that loads a model from the provided file path
OBJModel::OBJModel(const std::string &filepath) { loadModelFromFile(filepath); }

// Sets up the vertex buffer objects and vertex array object
void OBJModel::setupBuffers() {
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
               vertices.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
               indices.data(), GL_STATIC_DRAW);

  // Vertex positions
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
  glEnableVertexAttribArray(0);

  // Texture coordinates
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, texCoords));
  glEnableVertexAttribArray(1);

  // Vertex normals
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void *)offsetof(Vertex, normal));
  glEnableVertexAttribArray(2);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // Not needed anymore because data is now in GPU
  glDeleteBuffers(1, &ebo);
}

// Sets the shader material uniforms
void OBJModel::SetShaderMaterialUniforms(GLuint shaderProgram) {
  glUniform3fv(glGetUniformLocation(shaderProgram, "ka"), 1, material.ka);
  glUniform3fv(glGetUniformLocation(shaderProgram, "kd"), 1, material.kd);
  glUniform3fv(glGetUniformLocation(shaderProgram, "ks"), 1, material.ks);

  // Set the uniform for the texture sampler to the appropriate texture unit
  glUniform1i(glGetUniformLocation(shaderProgram, "u_DiffuseMap"),
              0); // Corresponds to GL_TEXTURE0
  glUniform1i(glGetUniformLocation(shaderProgram, "u_BumpMap"),
              1); // Corresponds to GL_TEXTURE1
  glUniform1i(glGetUniformLocation(shaderProgram, "u_SpecularMap"),
              2); // Corresponds to GL_TEXTURE2
}

// Renders the model by binding the VAO and drawing its elements
void OBJModel::render() const {
  glBindVertexArray(vao);

  if (material.map_kd.GetImage()) {
    glActiveTexture(GL_TEXTURE0);
    material.map_kd.Bind(0);
  }

  if (material.map_bump.GetImage()) {
    glActiveTexture(GL_TEXTURE1);
    material.map_kd.Bind(1);
  }

  if (material.map_ks.GetImage()) {
    glActiveTexture(GL_TEXTURE2);
    material.map_kd.Bind(2);
  }

  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()),
                 GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

// Loads the model data from the specified .obj file
void OBJModel::loadModelFromFile(const std::string &filepath) {
  std::ifstream objFile(filepath);
  if (!objFile.is_open()) {
    std::cerr << "Failed to open the OBJ file: " << filepath << std::endl;
    return;
  }

  std::string line;
  std::vector<glm::vec3> temp_vertices;
  std::vector<glm::vec2> temp_texCoords;
  std::vector<glm::vec3> temp_normals;

  while (std::getline(objFile, line)) {
    std::istringstream ss(line);
    std::string prefix;
    ss >> prefix;

    if (prefix == "mtllib") {
      std::string mtlFileName;
      ss >> mtlFileName;
      LoadMaterials(filepath.substr(0, filepath.find_last_of("/\\") + 1) +
                    mtlFileName);
    } else if (prefix == "usemtl") {
      std::string matName;
      ss >> matName;
    } else if (prefix == "v") {
      glm::vec3 vertex;
      ss >> vertex.x >> vertex.y >> vertex.z;
      temp_vertices.push_back(vertex);
    } else if (prefix == "vt") {
      glm::vec2 texCoord;
      ss >> texCoord.x >> texCoord.y;
      temp_texCoords.push_back(texCoord);
    } else if (prefix == "vn") {
      glm::vec3 normal;
      ss >> normal.x >> normal.y >> normal.z;
      temp_normals.push_back(normal);
    } else if (prefix == "f") {
      std::string vertex1, vertex2, vertex3;
      unsigned int vIndex[3], uvIndex[3], nIndex[3];
      char slash;

      ss >> vertex1 >> vertex2 >> vertex3;
      sscanf(vertex1.c_str(), "%d/%d/%d", &vIndex[0], &uvIndex[0], &nIndex[0]);
      sscanf(vertex2.c_str(), "%d/%d/%d", &vIndex[1], &uvIndex[1], &nIndex[1]);
      sscanf(vertex3.c_str(), "%d/%d/%d", &vIndex[2], &uvIndex[2], &nIndex[2]);

      for (int i = 0; i < 3; i++) {
        Vertex vertex;
        vertex.position = temp_vertices[vIndex[i] - 1];
        vertex.texCoords = temp_texCoords[uvIndex[i] - 1];
        vertex.normal = temp_normals[nIndex[i] - 1];
        vertices.push_back(vertex);
        indices.push_back(vertices.size() - 1);
      }

      // Define a list of offsets
      std::vector<glm::vec3> offsets = generateOffsetVectors(3);

      for (auto &offset : offsets) {
        for (int i = 0; i < 3; i++) {
          Vertex vertex;
          vertex.position = temp_vertices[vIndex[i] - 1] + offset;
          vertex.texCoords = temp_texCoords[uvIndex[i] - 1];
          vertex.normal = temp_normals[nIndex[i] - 1];
          vertices.push_back(vertex);
          indices.push_back(vertices.size() - 1);
        }
      }
    }
  }

  oriIndices = indices;
  optimizingIndices();

  std::cout << "The number of indices: " << indices.size() << std::endl;

  objFile.close();
  setupBuffers();
}

// Optimize the order of indices
void OBJModel::optimizingIndices() {
  std::vector<ForsythVertexIndexType> tmpIndices(indices.begin(),
                                                 indices.end());

  std::vector<ForsythVertexIndexType> optimizedIndices(indices.size());

  forsythReorderIndices(optimizedIndices.data(), tmpIndices.data(),
                        indices.size() / 3, vertices.size());

  optiIndices.resize(optimizedIndices.size());
  std::transform(optimizedIndices.begin(), optimizedIndices.end(),
                 indices.begin(), [](ForsythVertexIndexType idx) -> GLuint {
                   return static_cast<GLuint>(idx);
                 });
}

// To create more vertices to compare the performance
std::vector<glm::vec3> OBJModel::generateOffsetVectors(int maxOffset) {
  std::vector<glm::vec3> offsets;

  for (int x = -maxOffset; x <= maxOffset; ++x) {
    for (int y = -maxOffset; y <= maxOffset; ++y) {
      for (int z = -maxOffset; z <= maxOffset; ++z) {
        if (x != 0 || y != 0 || z != 0) {
          offsets.push_back(glm::vec3(x, y, z));
        }
      }
    }
  }

  return offsets;
}

// Loads the material properties from a .mtl file
void OBJModel::LoadMaterials(const std::string &mtlFilePath) {
  std::ifstream mtlFile(mtlFilePath);
  std::string line;

  if (!mtlFile.is_open()) {
    std::cerr << "Could not open MTL file at " << mtlFilePath << std::endl;
    return;
  }

  std::string directory =
      mtlFilePath.substr(0, mtlFilePath.find_last_of("/\\") + 1);

  while (getline(mtlFile, line)) {
    std::istringstream lineStream(line);
    std::string prefix;
    lineStream >> prefix;

    if (prefix == "Ns") {
      lineStream >> material.ns;
    } else if (prefix == "Ka") {
      lineStream >> material.ka[0] >> material.ka[1] >> material.ka[2];
    } else if (prefix == "Kd") {
      lineStream >> material.kd[0] >> material.kd[1] >> material.kd[2];
    } else if (prefix == "Ks") {
      lineStream >> material.ks[0] >> material.ks[1] >> material.ks[2];
    } else if (prefix == "Ni") {
      lineStream >> material.ni;
    } else if (prefix == "d") {
      lineStream >> material.d;
    } else if (prefix == "illum") {
      lineStream >> material.illum;
    } else if (prefix == "map_Kd") {
      std::string textureFile;
      lineStream >> textureFile;
      material.map_kd.LoadTexture(directory + textureFile);
    } else if (prefix == "map_Bump") {
      std::string textureFile;
      lineStream >> textureFile;
      material.map_bump.LoadTexture(directory + textureFile);
    } else if (prefix == "map_Ks") {
      std::string textureFile;
      lineStream >> textureFile;
      material.map_ks.LoadTexture(directory + textureFile);
    }
  }
}

// Reorder indices according to mode
void OBJModel::setCacheMode(const int mode) {
  if (mode == 1) {
    indices = oriIndices;
  } else if (mode == 2) {
    indices = optiIndices;
  } else if (mode == 3) {
    auto rng = std::default_random_engine(
        std::chrono::system_clock::now().time_since_epoch().count());
    std::shuffle(indices.begin(), indices.end(), rng);
  }

  std::cout << "First twenty indices" << std::endl;
  for (int i = 0; i < 20; i++) {
    std::cout << indices.at(i) << " ";
  }
  std::cout << std::endl;
}

// Destructor which cleans up the allocated buffers
OBJModel::~OBJModel() {
  glDeleteBuffers(1, &vbo);
  glDeleteBuffers(1, &ebo);
  glDeleteVertexArrays(1, &vao);
}
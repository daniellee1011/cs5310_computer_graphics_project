
/* Compilation on Linux:
 g++ -std=c++17 ./src/*.cpp -o prog -I ./include/ -I./../common/thirdparty/
 -lSDL2 -ldl
*/

// Third Party Libraries
#include <SDL2/SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

// C++ Standard Template Library (STL)
#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

// Our libraries
#include "Camera.hpp"
#include "OBJModel.hpp"
#include "Texture.hpp"

// vvvvvvvvvvvvvvvvvvvvvvvvvv Globals vvvvvvvvvvvvvvvvvvvvvvvvvv
// Globals generally are prefixed with 'g' in this application.

// Screen Dimensions
int gScreenWidth = 640;
int gScreenHeight = 480;
SDL_Window *gGraphicsApplicationWindow = nullptr;
SDL_GLContext gOpenGLContext = nullptr;

// Main loop flag
bool gQuit = false; // If this is quit = 'true' then the program terminates.

// shader
// The following stores the a unique id for the graphics pipeline
// program object that will be used for our OpenGL draw calls.
GLuint gGraphicsPipelineShaderProgram = 0;

// OpenGL Objects
// Vertex Array Object (VAO)
// Vertex array objects encapsulate all of the items needed to render an object.
// For example, we may have multiple vertex buffer objects (VBO) related to
// rendering one object. The VAO allows us to setup the OpenGL state to render
// that object using the correct layout and correct buffers with one call after
// being setup.
GLuint gVertexArrayObject = 0;
// Vertex Buffer Object (VBO)
// Vertex Buffer Objects store information relating to vertices (e.g. positions,
// normals, textures) VBOs are our mechanism for arranging geometry on the GPU.
GLuint gVertexBufferObject = 0;
// Index Buffer Object (IBO)
// This is used to store the array of indices that we want
// to draw from, when we do indexed drawing.
GLuint gIndexBufferObject = 0;

// Shaders
// Here we setup two shaders, a vertex shader and a fragment shader.
// At a minimum, every Modern OpenGL program needs a vertex and a fragment
// shader.
float g_uOffset = -2.0f;
float g_uRotate = 0.0f;

// Camera
Camera gCamera;

// Draw wireframe mode
GLenum gPolygonMode = GL_FILL;

// Obj file
OBJModel objModel;
std::string filepath;

// Texture
Texture gTexture;

// ^^^^^^^^^^^^^^^^^^^^^^^^ Globals ^^^^^^^^^^^^^^^^^^^^^^^^^^^

// vvvvvvvvvvvvvvvvvvv Error Handling Routines vvvvvvvvvvvvvvv
static void GLClearAllErrors() {
  while (glGetError() != GL_NO_ERROR) {
  }
}

// Returns true if we have an error
static bool GLCheckErrorStatus(const char *function, int line) {
  while (GLenum error = glGetError()) {
    std::cout << "OpenGL Error:" << error << "\tLine: " << line
              << "\tfunction: " << function << std::endl;
    return true;
  }
  return false;
}

#define GLCheck(x)                                                             \
  GLClearAllErrors();                                                          \
  x;                                                                           \
  GLCheckErrorStatus(#x, __LINE__);
// ^^^^^^^^^^^^^^^^^^^ Error Handling Routines ^^^^^^^^^^^^^^^

/**
 * LoadShaderAsString takes a filepath as an argument and will read line by line
 * a file and return a string that is meant to be compiled at runtime for a
 * vertex, fragment, geometry, tesselation, or compute shader. e.g.
 *       LoadShaderAsString("./shaders/filepath");
 *
 * @param filename Path to the shader file
 * @return Entire file stored as a single string
 */
std::string LoadShaderAsString(const std::string &filename) {
  // Resulting shader program loaded as a single string
  std::string result = "";

  std::string line = "";
  std::ifstream myFile(filename.c_str());

  if (myFile.is_open()) {
    while (std::getline(myFile, line)) {
      result += line + '\n';
    }
    myFile.close();
  }

  return result;
}

/**
 * CompileShader will compile any valid vertex, fragment, geometry, tesselation,
 *or compute shader. e.g. Compile a vertex shader:
 *CompileShader(GL_VERTEX_SHADER, vertexShaderSource); Compile a fragment
 *shader: 	CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
 *
 * @param type We use the 'type' field to determine which shader we are going to
 *compile.
 * @param source : The shader source code.
 * @return id of the shaderObject
 */
GLuint CompileShader(GLuint type, const std::string &source) {
  // Compile our shaders
  GLuint shaderObject;

  // Based on the type passed in, we create a shader object specifically for
  // that type.
  if (type == GL_VERTEX_SHADER) {
    shaderObject = glCreateShader(GL_VERTEX_SHADER);
  } else if (type == GL_FRAGMENT_SHADER) {
    shaderObject = glCreateShader(GL_FRAGMENT_SHADER);
  }

  const char *src = source.c_str();
  // The source of our shader
  glShaderSource(shaderObject, 1, &src, nullptr);
  // Now compile our shader
  glCompileShader(shaderObject);
  // Retrieve the result of our compilation
  int result;
  // Our goal with glGetShaderiv is to retrieve the compilation status
  glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &result);

  if (result == GL_FALSE) {
    int length;
    glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
    char *errorMessages = new char[length]; // Could also use alloca here.
    glGetShaderInfoLog(shaderObject, length, &length, errorMessages);

    if (type == GL_VERTEX_SHADER) {
      std::cout << "ERROR: GL_VERTEX_SHADER compilation failed!\n"
                << errorMessages << "\n";
    } else if (type == GL_FRAGMENT_SHADER) {
      std::cout << "ERROR: GL_FRAGMENT_SHADER compilation failed!\n"
                << errorMessages << "\n";
    }
    // Reclaim our memory
    delete[] errorMessages;

    // Delete our broken shader
    glDeleteShader(shaderObject);

    return 0;
  }

  return shaderObject;
}

/**
 * Creates a graphics program object (i.e. graphics pipeline) with a Vertex
 * Shader and a Fragment Shader
 *
 * @param vertexShaderSource Vertex source code as a string
 * @param fragmentShaderSource Fragment shader source code as a string
 * @return id of the program Object
 */
GLuint CreateShaderProgram(const std::string &vertexShaderSource,
                           const std::string &fragmentShaderSource) {

  // Create a new program object
  GLuint programObject = glCreateProgram();

  // Compile our shaders
  GLuint myVertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
  GLuint myFragmentShader =
      CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

  // Link our two shader programs together.
  // Consider this the equivalent of taking two .cpp files, and linking them
  // into one executable file.
  glAttachShader(programObject, myVertexShader);
  glAttachShader(programObject, myFragmentShader);
  glLinkProgram(programObject);

  // Validate our program
  glValidateProgram(programObject);

  // Once our final program Object has been created, we can
  // detach and then delete our individual shaders.
  glDetachShader(programObject, myVertexShader);
  glDetachShader(programObject, myFragmentShader);
  // Delete the individual shaders once we are done
  glDeleteShader(myVertexShader);
  glDeleteShader(myFragmentShader);

  return programObject;
}

/**
 * Create the graphics pipeline
 *
 * @return void
 */
void CreateGraphicsPipeline() {
  std::string vertexShaderSource = LoadShaderAsString("./shaders/vert.glsl");
  std::string fragmentShaderSource = LoadShaderAsString("./shaders/frag.glsl");

  gGraphicsPipelineShaderProgram =
      CreateShaderProgram(vertexShaderSource, fragmentShaderSource);
}

/**
 * Initialization of the graphics application. Typically this will involve
 * setting up a window and the OpenGL Context (with the appropriate version)
 *
 * @return void
 */
void InitializeProgram() {
  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::cout << "SDL could not initialize! SDL Error: " << SDL_GetError()
              << "\n";
    exit(1);
  }

  // Setup the OpenGL Context
  // Use OpenGL 4.1 core or greater
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  // We want to request a double buffer for smooth updating.
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  // Create an application window using OpenGL that supports SDL
  gGraphicsApplicationWindow = SDL_CreateWindow(
      "Textured", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
      gScreenWidth, gScreenHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

  // Check if Window did not create.
  if (gGraphicsApplicationWindow == nullptr) {
    std::cout << "Window could not be created! SDL Error: " << SDL_GetError()
              << "\n";
    exit(1);
  }

  // Create an OpenGL Graphics Context
  gOpenGLContext = SDL_GL_CreateContext(gGraphicsApplicationWindow);
  if (gOpenGLContext == nullptr) {
    std::cout << "OpenGL context could not be created! SDL Error: "
              << SDL_GetError() << "\n";
    exit(1);
  }

  // Initialize GLAD Library
  if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
    std::cout << "glad did not initialize" << std::endl;
    exit(1);
  }
}

/**
 * Setup your geometry during the vertex specification step
 *
 * @return void
 */
void VertexSpecification() { objModel.loadModelFromFile(filepath); }

/**
 * PreDraw
 * Typically we will use this for setting some sort of 'state'
 * Note: some of the calls may take place at different stages (post-processing)
 * of the pipeline.
 * @return void
 */
void PreDraw() {
  // Disable depth test and face culling.
  glEnable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  // Set the polygon fill mode
  glPolygonMode(GL_FRONT_AND_BACK, gPolygonMode);

  // Enable texture mapping
  glEnable(GL_TEXTURE_2D);

  // Initialize clear color
  // This is the background of the screen.
  glViewport(0, 0, gScreenWidth, gScreenHeight);
  glClearColor(0.1f, 0.1f, 0.1f, 1.f);

  // Clear color buffer and Depth Buffer
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

  // Use our shader
  // Note: Make sure to call 'use program' before looking up uniform values,
  // otherwise 			 you may not get anything returned.
  // See:
  // https://www.khronos.org/opengl/wiki/GLSL_:_common_mistakes#glUniform_doesn't_work
  glUseProgram(gGraphicsPipelineShaderProgram);

  // Model transformation by translating our object into world space
  glm::mat4 model =
      glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, g_uOffset));

  // Update our model matrix by applying a rotation after our translation
  model =
      glm::rotate(model, glm::radians(g_uRotate), glm::vec3(0.0f, 1.0f, 0.0f));

  // Retrieve our location of our Model Matrix
  GLint u_ModelMatrixLocation =
      glGetUniformLocation(gGraphicsPipelineShaderProgram, "model");
  if (u_ModelMatrixLocation >= 0) {
    glUniformMatrix4fv(u_ModelMatrixLocation, 1, GL_FALSE, &model[0][0]);
  } else {
    std::cout << "Could not find model, maybe a mispelling?\n";
    exit(EXIT_FAILURE);
  }

  // Update the View Matrix
  GLint u_ViewMatrixLocation =
      glGetUniformLocation(gGraphicsPipelineShaderProgram, "view");
  if (u_ViewMatrixLocation >= 0) {
    glm::mat4 viewMatrix = gCamera.GetViewMatrix();
    glUniformMatrix4fv(u_ViewMatrixLocation, 1, GL_FALSE, &viewMatrix[0][0]);
  } else {
    std::cout << "Could not find view, maybe a mispelling?\n";
    exit(EXIT_FAILURE);
  }

  // Projection matrix (in perspective)
  glm::mat4 perspective =
      glm::perspective(glm::radians(45.0f),
                       (float)gScreenWidth / (float)gScreenHeight, 0.1f, 10.0f);

  // Retrieve our location of our perspective matrix uniform
  GLint u_ProjectionLocation =
      glGetUniformLocation(gGraphicsPipelineShaderProgram, "projection");
  if (u_ProjectionLocation >= 0) {
    glUniformMatrix4fv(u_ProjectionLocation, 1, GL_FALSE, &perspective[0][0]);
  } else {
    std::cout << "Could not find projection, maybe a mispelling?\n";
    exit(EXIT_FAILURE);
  }

  // Retrieve camera position and send it to the fragment shader
  glm::vec3 cameraPosition =
      glm::vec3(gCamera.GetEyeXPosition(), gCamera.GetEyeYPosition(),
                gCamera.GetEyeZPosition());

  GLint cameraPosLocation =
      glGetUniformLocation(gGraphicsPipelineShaderProgram, "u_CameraPosition");
  if (cameraPosLocation >= 0) {
    glUniform3f(cameraPosLocation, cameraPosition.x, cameraPosition.y,
                cameraPosition.z);
  } else {
    std::cout << "Could not find u_CameraPosition, maybe a mispelling?\n";
    exit(EXIT_FAILURE);
  }

  objModel.SetShaderMaterialUniforms(gGraphicsPipelineShaderProgram);
}

/**
 * Draw
 * The render function gets called once per loop.
 * Typically this includes 'glDraw' related calls, and the relevant setup of
 * buffers for those calls.
 *
 * @return void
 */
std::vector<double> frameTimes;
void Draw() {
  const size_t maxFrameSamples = 1000;
  auto startTime = std::chrono::high_resolution_clock::now();

  objModel.render();

  auto endTime = std::chrono::high_resolution_clock::now();
  frameTimes.push_back(
      std::chrono::duration<double>(endTime - startTime).count());
  if (frameTimes.size() % maxFrameSamples == 0) {
    double sum = std::accumulate(frameTimes.begin(), frameTimes.end(), 0.0);
    double averageFrameTime = sum / frameTimes.size();
    double averageFPS = 1.0 / averageFrameTime;
    std::cout << "Average Frame Time per 1000 frames: " << averageFrameTime
              << std::endl;
    std::cout << "Average FPS per 1000 frames: " << averageFPS << std::endl;
    frameTimes.clear();
  }

  // Stop using our current graphics pipeline
  // Note: This is not necessary if we only have one graphics pipeline.
  glUseProgram(0);
}

/**
 * Helper Function to get OpenGL Version Information
 *
 * @return void
 */
void getOpenGLVersionInfo() {
  std::cout << "Vendor: " << glGetString(GL_VENDOR) << "\n";
  std::cout << "Renderer: " << glGetString(GL_RENDERER) << "\n";
  std::cout << "Version: " << glGetString(GL_VERSION) << "\n";
  std::cout << "Shading language: " << glGetString(GL_SHADING_LANGUAGE_VERSION)
            << "\n";
}

/**
 * Function called in the Main application loop to handle user input
 *
 * @return void
 */
bool KeyPressed1 = false;
bool KeyPressed2 = false;
bool KeyPressed3 = false;
bool KeyPressed7 = false;
bool KeyPressed8 = false;
bool KeyPressed9 = false;
bool KeyPressed0 = false;
void Input() {
  // Event handler that handles various events in SDL
  // that are related to input and output
  SDL_Event e;
  // Handle events on queue
  while (SDL_PollEvent(&e) != 0) {
    // If users posts an event to quit
    // An example is hitting the "x" in the corner of the window.
    if (e.type == SDL_QUIT) {
      std::cout << "Goodbye! (Leaving MainApplicationLoop())" << std::endl;
      gQuit = true;
    }
    if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
      std::cout << "ESC: Goodbye! (Leaving MainApplicationLoop())" << std::endl;
      gQuit = true;
    }
  }

  // Retrieve keyboard state
  const Uint8 *state = SDL_GetKeyboardState(NULL);
  if (state[SDL_SCANCODE_UP]) {
    g_uOffset += 0.01f;
    std::cout << "g_uOffset: " << g_uOffset << std::endl;
  }
  if (state[SDL_SCANCODE_DOWN]) {
    g_uOffset -= 0.01f;
    std::cout << "g_uOffset: " << g_uOffset << std::endl;
  }
  if (state[SDL_SCANCODE_LEFT]) {
    g_uRotate -= 1.0f;
    std::cout << "g_uRotate: " << g_uRotate << std::endl;
  }
  if (state[SDL_SCANCODE_RIGHT]) {
    g_uRotate += 1.0f;
    std::cout << "g_uRotate: " << g_uRotate << std::endl;
  }

  // Vertex Cache Optimiziation Mode on/off
  if (state[SDL_SCANCODE_1] && !KeyPressed1) {
    std::cout << "Original indices!" << std::endl;
    objModel.setCacheMode(1);
    KeyPressed1 = true;
  } else if (!state[SDL_SCANCODE_1]) {
    KeyPressed1 = false;
  }
  if (state[SDL_SCANCODE_2] && !KeyPressed2) {
    std::cout << "Optimized indices!" << std::endl;
    objModel.setCacheMode(2);
    KeyPressed2 = true;
  } else if (!state[SDL_SCANCODE_2]) {
    KeyPressed2 = false;
  }
  if (state[SDL_SCANCODE_3] && !KeyPressed3) {
    std::cout << "Randomized indices!" << std::endl;
    objModel.setCacheMode(3);
    KeyPressed3 = true;
  } else if (!state[SDL_SCANCODE_3]) {
    KeyPressed3 = false;
  }

  // Switch obj file to render
  if (state[SDL_SCANCODE_7] && !KeyPressed7) {
    std::cout << "Tree object!" << std::endl;
    filepath = "./../common/objects/tree_3/HandpaintedTree.obj";
    objModel.loadModelFromFile(filepath);
    KeyPressed7 = true;
  } else if (!state[SDL_SCANCODE_7]) {
    KeyPressed7 = false;
  }
  if (state[SDL_SCANCODE_8] && !KeyPressed8) {
    std::cout << "Cube object!" << std::endl;
    filepath = "./../common/objects/textured_cube/cube.obj";
    objModel.loadModelFromFile(filepath);
    KeyPressed8 = true;
  } else if (!state[SDL_SCANCODE_8]) {
    KeyPressed8 = false;
  }
  if (state[SDL_SCANCODE_9] && !KeyPressed9) {
    std::cout << "Chapel object!" << std::endl;
    filepath = "./../common/objects/chapel/chapel_obj.obj";
    objModel.loadModelFromFile(filepath);
    KeyPressed9 = true;
  } else if (!state[SDL_SCANCODE_9]) {
    KeyPressed9 = false;
  }
  if (state[SDL_SCANCODE_0] && !KeyPressed0) {
    std::cout << "House object!" << std::endl;
    filepath = "./../common/objects/house/house_obj.obj";
    objModel.loadModelFromFile(filepath);
    KeyPressed0 = true;
  } else if (!state[SDL_SCANCODE_0]) {
    KeyPressed0 = false;
  }

  // Camera
  // Update our position of the camera
  if (state[SDL_SCANCODE_W]) {
    gCamera.MoveForward(0.01f);
  }
  if (state[SDL_SCANCODE_S]) {
    gCamera.MoveBackward(0.01f);
  }
  if (state[SDL_SCANCODE_A]) {
    gCamera.MoveLeft(0.01f);
  }
  if (state[SDL_SCANCODE_D]) {
    gCamera.MoveRight(0.01f);
  }
  if (state[SDL_SCANCODE_TAB]) {
    SDL_Delay(250); // This is hacky in the name of simplicity,
                    // but we just delay the
                    // system by a few milli-seconds to process the
                    // keyboard input once at a time.
    if (gPolygonMode == GL_FILL) {
      gPolygonMode = GL_LINE;
    } else {
      gPolygonMode = GL_FILL;
    }
  }
  // Update the mouse look of the camera
  // Center the mouse in the window
  int mouseX, mouseY;
  SDL_GetGlobalMouseState(&mouseX, &mouseY);
  gCamera.MouseLook(mouseX, mouseY);
}

/**
 * Main Application Loop
 * This is an infinite loop in our graphics application
 *
 * @return void
 */
void MainLoop() {

  // For the mouse look
  // Nice to center mouse in the window
  SDL_WarpMouseInWindow(gGraphicsApplicationWindow, 640 / 2, 480 / 2);

  // While application is running
  while (!gQuit) {
    // Handle Input
    Input();
    // Setup anything (i.e. OpenGL State) that needs to take
    // place before draw calls
    PreDraw();
    // Draw Calls in OpenGL
    // When we 'draw' in OpenGL, this activates the graphics pipeline.
    // i.e. when we use glDrawElements or glDrawArrays,
    //      The pipeline that is utilized is whatever 'glUseProgram' is
    //      currently binded.
    Draw();
    // Update screen of our specified window
    SDL_GL_SwapWindow(gGraphicsApplicationWindow);
  }
}

/**
 * The last function called in the program
 * This functions responsibility is to destroy any global
 * objects in which we have create dmemory.
 *
 * @return void
 */
void CleanUp() {
  // Destroy our SDL2 Window
  SDL_DestroyWindow(gGraphicsApplicationWindow);
  gGraphicsApplicationWindow = nullptr;

  // Delete our OpenGL Objects
  //   glDeleteBuffers(1, &gVertexBufferObject);
  //   glDeleteVertexArrays(1, &gVertexArrayObject);

  // Delete our Graphics pipeline
  glDeleteProgram(gGraphicsPipelineShaderProgram);

  // Quit SDL subsystems
  SDL_Quit();
}

/**
 * The entry point into our C++ programs.
 *
 * @return program status
 */
int main(int argc, char *args[]) {
  std::cout << "Use arrow keys to move and rotate\n";
  std::cout << "Use wasd to move\n";
  std::cout << "Use TAB to toggle wireframe\n";
  std::cout << "Press ESC to quit\n";

  filepath = "./../common/objects/tree_3/HandpaintedTree.obj";

  // 1. Setup the graphics program
  InitializeProgram();

  // 2. Setup our geometry
  VertexSpecification();

  // 3. Create our graphics pipeline
  // 	- At a minimum, this means the vertex and fragment shader
  CreateGraphicsPipeline();

  // 4. Call the main application loop
  MainLoop();

  // 5. Call the cleanup function when our program terminates
  CleanUp();

  return 0;
}

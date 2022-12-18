#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <vector>           // Vector for list-like features
#include <cmath>

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#include <camera.h>         // Camera Implementation

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions

using namespace std;        // Standard Namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// WINDOW CONSTS
const char* const WINDOW_TITLE = "Assignment 7-1";
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

struct GLMesh // Mesh Data
{
    GLuint vao;           // Handle for the vertex array object
    GLuint vbo;           // Handle for the vertex buffer object
    GLuint nVertices;     // Number of indices of the mesh
    GLuint textureId;     // Image for mesh
};

GLFWwindow* gWindow = nullptr; // Main GLFW window
vector<GLMesh> gMeshVector; // Vector of all the meshes

// Texture
glm::vec2 gUVScale(1.0f, 1.0f);
GLint gTexWrapMode = GL_REPEAT;

// Shader Programs
GLuint gMeshProgramId; // Mesh Shader Id
GLuint gLightProgramId; // Light Shader Id

// Mesh Color
glm::vec3 gObjectColor(1.f, 0.2f, 0.0f);
glm::vec3 gLightColor(1.0f, 1.0f, 1.0f);

// Light position and scale
glm::vec3 gLightPosition(0.0f, 2.0f, 3.0f);

// Camera
glm::vec3 gCameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 gCameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 gCameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
float gLastX = WINDOW_WIDTH / 2.0f;
float gLastY = WINDOW_HEIGHT / 2.0f;
bool gFirstMouse = true;

bool gIsPPressed = false; // Prevents double-tapping P

// Time
float gDeltaTime = 0.0f; // time between current frame and last frame
float gLastFrame = 0.0f;

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
void addCounter(int& current, int max, int increment);
bool createTexture(const char* filename, GLuint& textureId);
void flipImageVertically(unsigned char* image, int width, int height, int channels);
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UCreateCube(float x, float y, float z, float w, float h, float l, const char* filename);
void UCreateCylinder(float x, float y, float z, float r, float h, const char* filename);
void UCreatePlane(
    float x1, float y1, float z1, 
    float x2, float y2, float z2,
    float x3, float y3, float z3,
    float x4, float y4, float z4, const char* filename
);
void UCreatePyramid(float x, float y, float z, float w, float h, float l, const char* filename);
void UDestroyMesh();
void UDestroyTexture();
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram();

/* Cube Vertex Shader Source Code*/
const GLchar* meshVertexShaderSource = GLSL(440,
	layout(location = 0) in vec3 position; // VAP position 0 for vertex position data
	layout(location = 1) in vec3 normal; // VAP position 1 for normals
	layout(location = 2) in vec2 textureCoordinate;

	out vec3 vertexNormal; // For outgoing normals to fragment shader
	out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
	out vec2 vertexTextureCoordinate;

	//Uniform / Global variables for the  transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main()
	{
	    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates

	    vertexFragmentPos = vec3(model * vec4(position, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

	    vertexNormal = mat3(transpose(inverse(model))) * normal; // get normal vectors in world space only and exclude normal translation properties
	    vertexTextureCoordinate = textureCoordinate;
	}
);

/* Cube Fragment Shader Source Code*/
const GLchar* meshFragmentShaderSource = GLSL(440,
    in vec3 vertexNormal; // For incoming normals
	in vec3 vertexFragmentPos; // For incoming fragment position
	in vec2 vertexTextureCoordinate;
	out vec4 fragmentColor; // For outgoing cube color to the GPU

	// Uniform / Global variables for object color, light color, light position, and camera/view position
	uniform vec3 objectColor;
	uniform vec3 lightColor;
	uniform vec3 lightPos;
	uniform vec3 viewPosition;
	uniform sampler2D uTexture; // Useful when working with multiple textures
	uniform vec2 uvScale;

	void main()
	{
	    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

	    //Calculate Ambient lighting*/
	    float ambientStrength = .4f; // Set ambient or global lighting strength
	    vec3 ambient = ambientStrength * lightColor; // Generate ambient light color

	    //Calculate Diffuse lighting*/
	    vec3 norm = normalize(vertexNormal); // Normalize vectors to 1 unit
	    vec3 lightDirection = normalize(lightPos - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
	    float impact = max(dot(norm, lightDirection), 0.0);// Calculate diffuse impact by generating dot product of normal and light
	    vec3 diffuse = impact * lightColor; // Generate diffuse light color

	    //Calculate Specular lighting*/
	    float specularIntensity = 0.8f; // Set specular light strength
	    float highlightSize = 16.0f; // Set specular highlight size
	    vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
	    vec3 reflectDir = reflect(-lightDirection, norm);// Calculate reflection vector
	    //Calculate specular component
	    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
	    vec3 specular = specularIntensity * specularComponent * lightColor;

	    // Texture holds the color to be used for all three components
	    vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);

	    // Calculate phong result
	    vec3 phong = (ambient + diffuse + specular) * textureColor.xyz;

	    fragmentColor = vec4(phong, 1.0); // Send lighting results to GPU
	}
);

/* Lamp Shader Source Code*/
const GLchar* lightVertexShaderSource = GLSL(440,
    layout(location = 0) in vec3 position; // VAP position 0 for vertex position data

    //Uniform / Global variables for the  transform matrices
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main()
	{
	    gl_Position = projection * view * model * vec4(position, 1.0f); // Transforms vertices into clip coordinates
	}
);

/* Fragment Shader Source Code*/
const GLchar* lightFragmentShaderSource = GLSL(440,
    out vec4 fragmentColor; // For outgoing lamp color (smaller cube) to the GPU

	void main()
	{
	    fragmentColor = vec4(1.0f); // Set color to white (1.0f,1.0f,1.0f) with alpha 1.0
	}
);

/*Counts a counter and resets to 0 on overlap*/
void addCounter(int& current, int max, int increment = 0)
{
    current += increment;
    if (current > max)
        current -= max;
    if (current < 0)
        current += max;
}

/*Generate and load the texture*/
bool createTexture(const char* filename, GLuint& textureId)
{
    int width, height, channels;
    unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
    if (image)
    {
        flipImageVertically(image, width, height, channels);

        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        // set the texture wrapping parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (channels == 3)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
        else if (channels == 4)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
        else
        {
            cout << "Not implemented to handle image with " << channels << " channels" << endl;
            return false;
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(image);
        glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

        return true;
    }

    // Error loading the image
    return false;
}

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
    for (int j = 0; j < height / 2; ++j)
    {
        int index1 = j * width * channels;
        int index2 = (height - 1 - j) * width * channels;

        for (int i = width * channels; i > 0; --i)
        {
            unsigned char tmp = image[index1];
            image[index1] = image[index2];
            image[index2] = tmp;
            ++index1;
            ++index2;
        }
    }
}

int main(int argc, char* argv[])
{
    if (!UInitialize(argc, argv, &gWindow))
        return EXIT_FAILURE;

    // Create the meshs

    // Desk
    const char * woodFilePath = "resources/Balsa_Wood_Texture.jpg";
    const char * hardwoodFilePath = "resources/hardwood.jpg";
    const char* paperFilePath = "resources/Free_crumpled_paper_texture_for_layers_(2978651767).jpg";
    UCreateCube(0.0f, 0.0f, 0.0f, 0.9f, 0.1f, 0.9f, woodFilePath);  // flat desktop
    UCreateCube(1.0f, 0.0f, 0.0f, 0.1f, 1.2f, 1.0f, woodFilePath);  // left wall
    UCreateCube(0.0f, 0.0f, 1.0f, 1.1f, 1.2f, .1f, woodFilePath);   // back wall
    UCreateCube(-1.0f, 0.0f, 0.0f, 0.1f, 1.2f, 1.0f, woodFilePath); // right wall
    UCreatePlane(1.11f, 0.0f, 0.6f, 1.11f, 0.0f, 0.8f, 1.11f, -0.4f, 0.8f, 1.11f, -0.4f, 0.6f, hardwoodFilePath); // handle
    UCreatePlane(-.89f, 0.8f, 0.0f,-.89f, 0.8f, 0.5f,-.89f, 0.3, 0.5f,-.89f, 0.3, 0.0f, paperFilePath); // paper on desk

    // Room
    const char* floorFilePath = "resources/black-and-white.jpg";
    UCreatePlane(10.0f, -1.0f, -10.0f, -10.0f, -1.0f, -10.0f, -10.0f, -1.0f, 10.0f, 10.0f, -1.0f, 10.0f, floorFilePath); // paper on desk

    // Trashcan
    const char* trashcanFilePath = "resources/blue-leather.jpg";
    glm::vec3 tPos = glm::vec3(1.4, -.95, 0);
    UCreateCube(0.0f + tPos.x, 0.0f + tPos.y, 0.0f + tPos.z, 0.2f, 0.05f, 0.4f, trashcanFilePath);  // bottom
    UCreateCube(-0.15f + tPos.x, 0.45f + tPos.y, 0.0f + tPos.z, 0.05f, 0.4f, 0.4f, trashcanFilePath);  // left
    UCreateCube(0.15f + tPos.x, 0.45f + tPos.y, 0.0f + tPos.z, 0.05f, 0.4f, 0.4f, trashcanFilePath);  // right
    UCreateCube(0.0f + tPos.x, 0.45f + tPos.y, .35f + tPos.z, 0.1f, 0.4f, 0.05f, trashcanFilePath);  // front
    UCreateCube(0.0f + tPos.x, 0.45f + tPos.y, -.35f + tPos.z, 0.1f, 0.4f, 0.05f, trashcanFilePath);  // back

    // Chair
    const char* metalFilePath = "resources/metal.jpg";
    const char* chairFilePath = "resources/texture-floor-asphalt-pattern-line-brown-1270308-pxhere.com.jpg";
    glm::vec3 cPos = glm::vec3(0.0f, -0.4f, -1.5f);
    UCreateCylinder(0.0f + cPos.x, -0.3f + cPos.y, 0.0f + cPos.z, 0.1f, 0.4f, metalFilePath);  // seat leg
    UCreateCube(0.0f + cPos.x, -0.55f + cPos.y, 0.0f + cPos.z, 0.5f, 0.05f, 0.1f, metalFilePath);  // seat leg x
    UCreateCube(0.0f + cPos.x, -0.55f + cPos.y, 0.0f + cPos.z, 0.1f, 0.05f, 0.5f, metalFilePath);  // seat leg z
    UCreateCube(0.0f + cPos.x, 0.0f + cPos.y, 0.0f + cPos.z, 0.5f, 0.1f, 0.5f, chairFilePath);  // seat
    UCreateCylinder(-0.2f + cPos.x, 0.3f + cPos.y, -0.4f + cPos.z, 0.05f, 0.4f, metalFilePath);  // back leg
    UCreateCylinder(0.2f + cPos.x, 0.3f + cPos.y, -0.4f + cPos.z, 0.05f, 0.4f, metalFilePath);  // back leg 2
    UCreateCube(0.0f + cPos.x, 0.9f + cPos.y, -0.4f + cPos.z, 0.5f, 0.4f, 0.1f, chairFilePath);  // back

    // Computer
    UCreatePlane(-0.4f, 0.11f, 0.0f, -0.8f, 0.11f, 0.0f, -0.8f, 0.11f, -0.4f, -0.4f, 0.11f, -0.4f, trashcanFilePath); // mousepad
    UCreatePyramid(-0.6f, 0.15f, -0.2f, 0.075f, 0.05f, 0.125f, chairFilePath);  // mouse
    UCreateCube(0.2f, 0.1f, -0.3f, 0.4f, 0.025f, 0.2f, chairFilePath);  // keyboard
    UCreateCube(0.2f, 0.4f, 0.1f, 0.3f, 0.3f, 0.1f, chairFilePath);  // computer

	// Create the shader program
    if (!UCreateShaderProgram(meshVertexShaderSource, meshFragmentShaderSource, gMeshProgramId))
        return EXIT_FAILURE;

    if (!UCreateShaderProgram(lightVertexShaderSource, lightFragmentShaderSource, gLightProgramId))
        return EXIT_FAILURE;

    glUseProgram(gMeshProgramId);
    glUniform1i(glGetUniformLocation(gMeshProgramId, "uTexture"), 0); // Set texture unit

    // Sets the background color of the window to black (it will be implicitely used by glClear)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(gWindow))
    {
        // per-frame timing
        // --------------------
        float currentFrame = glfwGetTime();
        gDeltaTime = currentFrame - gLastFrame;
        gLastFrame = currentFrame;

        // input
        // -----
        UProcessInput(gWindow);

        // Render this frame
        URender();

        glfwPollEvents();
    }

    UDestroyMesh();    // Release mesh data
    UDestroyTexture(); // Release texture
    UDestroyShaderProgram(); // Release shader programs

    exit(EXIT_SUCCESS); // Terminates the program successfully
}

void UCreateCylinder(float x, float y, float z, float r, float h, const char* filename)
{
    struct Vert {
        GLfloat x;
        GLfloat y;
        GLfloat z;
        GLfloat nx;
        GLfloat ny;
        GLfloat nz;
        GLfloat tx;
        GLfloat ty;
        Vert(GLfloat _x, GLfloat _y, GLfloat _z, GLfloat _nx, GLfloat _ny, GLfloat _nz, GLfloat _tx, GLfloat _ty)
        {
            x = _x;
            y = _y;
            z = _z;
            nx = _nx;
            ny = _ny;
            nz = _nz;
            tx = _tx;
            ty = _ty;
        }
    };

    vector<Vert> vertVector;
    const float pi = 3.14159f;
    const int nEdges = 12;
    const int nVerts = nEdges * 6 * (3 + 3 + 2); // nEdges, Sides * 6 for the vertex per face, 3 normal map, 2 texture mapping
    GLfloat verts[nVerts];
    int currentEdge = 1;
    for (int i = 0; i < nEdges * 6; i++)
    {
        switch (i % 6)
        {
        case 0:
            vertVector.push_back(Vert(x + r * sin(2 * pi * currentEdge / nEdges), y + h / 2, z + r * cos(2 * pi * currentEdge / nEdges), 0.0f, 0.0f, 0.0f, 0.0f, 1.0f));
            break;
        case 1:
            vertVector.push_back(Vert(x + r * sin(2 * pi * currentEdge / nEdges), y - h / 2, z + r * cos(2 * pi * currentEdge / nEdges), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
            break;
        case 2:
            addCounter(currentEdge, nEdges, 1);
            vertVector.push_back(Vert(x + r * sin(2 * pi * currentEdge / nEdges), y + h / 2, z + r * cos(2 * pi * currentEdge / nEdges), 0.0f, 0.0f, 0.0f, 1.0f, 1.0f));
            break;
        case 3:
            addCounter(currentEdge, nEdges, -1);
            vertVector.push_back(Vert(x + r * sin(2 * pi * currentEdge / nEdges), y - h / 2, z + r * cos(2 * pi * currentEdge / nEdges), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
            break;
        case 4:
            addCounter(currentEdge, nEdges, 1);
            vertVector.push_back(Vert(x + r * sin(2 * pi * currentEdge / nEdges), y + h / 2, z + r * cos(2 * pi * currentEdge / nEdges), 0.0f, 0.0f, 0.0f, 1.0f, 1.0f));
            break;
        case 5:
            vertVector.push_back(Vert(x + r * sin(2 * pi * currentEdge / nEdges), y - h / 2, z + r * cos(2 * pi * currentEdge / nEdges), 0.0f, 0.0f, 0.0f, 1.0f, 0.0f));
            break;
        }
    }

    for (int i = 0; i < nVerts; i++)
    {
        Vert vert = vertVector[floor(i / 8)];
        switch (i % 8)
        {
        case 0:
            verts[i] = vert.x;
            break;
        case 1:
            verts[i] = vert.y;
            break;
        case 2:
            verts[i] = vert.z;
            break;
        case 3:
            verts[i] = vert.nx;
            break;
        case 4:
            verts[i] = vert.ny;
            break;
        case 5:
            verts[i] = vert.nz;
        case 6:
            verts[i] = vert.tx;
            break;
        case 7:
            verts[i] = vert.ty;
            break;
        }
    }

    GLMesh mesh;
    if (!createTexture(filename, mesh.textureId))
    {
        cout << "Failed to load texture " << filename << endl;
    }

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    gMeshVector.push_back(mesh);
}

void UCreateCube(float x, float y, float z, float w, float h, float l, const char* filename)
{
    float s = 0.0f;
    float f = 1.0f;
    GLfloat verts[] = {
        // pos             // normal
        x + w, y + h, z + l, 0.0f, 0.0f, -1.0f, f, f,
        x + w, y - h, z + l, 0.0f, 0.0f, -1.0f, f, s,
        x - w, y + h, z + l, 0.0f, 0.0f, -1.0f, s, f,

        x + w, y - h, z + l, 0.0f, 0.0f, -1.0f, f, s,
        x - w, y - h, z + l, 0.0f, 0.0f, -1.0f, s, s,
        x - w, y + h, z + l, 0.0f, 0.0f, -1.0f, s, f,

        x + w, y + h, z + l, 1.0f, 0.0f, 0.0f, f, f,
        x + w, y - h, z + l, 1.0f, 0.0f, 0.0f, s, f,
        x + w, y - h, z - l, 1.0f, 0.0f, 0.0f, s, s,

        x + w, y + h, z + l, 1.0f, 0.0f, 0.0f, f, f,
        x + w, y - h, z - l, 1.0f, 0.0f, 0.0f, s, s,
        x + w, y + h, z - l, 1.0f, 0.0f, 0.0f, f, s,

        x + w, y + h, z + l, 0.0f, 1.0f, 0.0f, f, f,
        x + w, y + h, z - l, 0.0f, 1.0f, 0.0f, f, s,
        x - w, y + h, z - l, 0.0f, 1.0f, 0.0f, s, s,

        x + w, y + h, z + l, 0.0f, 1.0f, 0.0f, f, f,
        x - w, y + h, z + l, 0.0f, 1.0f, 0.0f, s, f,
        x - w, y + h, z - l, 0.0f, 1.0f, 0.0f, s, s,

        x + w, y - h, z - l, 0.0f, 0.0f,-1.0f, f, s,
        x + w, y + h, z - l, 0.0f, 0.0f,-1.0f, f, f,
        x - w, y + h, z - l, 0.0f, 0.0f,-1.0f, s, f,

        x + w, y - h, z - l, 0.0f, 0.0f,-1.0f, f, s,
        x - w, y + h, z - l, 0.0f, 0.0f,-1.0f, s, f,
        x - w, y - h, z - l, 0.0f, 0.0f,-1.0f, s, s,

        x - w, y - h, z + l,-1.0f, 0.0f, 0.0f, s, f,
        x - w, y + h, z + l,-1.0f, 0.0f, 0.0f, f, f,
        x - w, y + h, z - l,-1.0f, 0.0f, 0.0f, f, s,

        x - w, y - h, z + l,-1.0f, 0.0f, 0.0f, s, f,
        x - w, y + h, z - l,-1.0f, 0.0f, 0.0f, f, s,
        x - w, y - h, z - l,-1.0f, 0.0f, 0.0f, s, s,

        x + w, y - h, z + l, 0.0f,-1.0f, 0.0f, f, f,
        x + w, y - h, z - l, 0.0f,-1.0f, 0.0f, f, s,
        x - w, y - h, z - l, 0.0f,-1.0f, 0.0f, s, s,

        x + w, y - h, z + l, 0.0f,-1.0f, 0.0f, f, f,
        x - w, y - h, z + l, 0.0f,-1.0f, 0.0f, s, f,
        x - w, y - h, z - l, 0.0f,-1.0f, 0.0f, s, s,
    };

    GLMesh mesh;
    if (!createTexture(filename, mesh.textureId))
    {
        cout << "Failed to load texture " << filename << endl;
    }

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    gMeshVector.push_back(mesh);
}

/* 
Creates a plane with the rule that point 1 and point 3 are on the opposite sides of the square.
The edges go as 1-2-3 and 1-4-3.  It should look like:
    1 ---- 2
    |      |
    |      |
    4 ---- 3
*/
void UCreatePlane(
    float x1, float y1, float z1, 
    float x2, float y2, float z2,
    float x3, float y3, float z3,
    float x4, float y4, float z4, const char* filename)
{
    float s = 0.0f;
    float f = 1.0f;
    GLfloat verts[] = {
        // pos          // normal
        x1, y1, z1,  1.0f, 0.0f, 0.0f,  s, s,
        x2, y2, z2,  1.0f, 0.0f, 0.0f,  s, f,
        x3, y3, z3,  1.0f, 0.0f, 0.0f,  f, f,
        x1, y1, z1,  1.0f, 0.0f, 0.0f,  s, s,
        x4, y4, z4,  1.0f, 0.0f, 0.0f,  f, s,
        x3, y3, z3,  1.0f, 0.0f, 0.0f,  f, f,
    };

    GLMesh mesh;
    if (!createTexture(filename, mesh.textureId))
    {
        cout << "Failed to load texture " << filename << endl;
    }

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    gMeshVector.push_back(mesh);
}

// Creates a square pyramid
void UCreatePyramid(float x, float y, float z, float w, float h, float l, const char* filename)
{
    GLfloat verts[] = {
        // pos             // normal
        x - w, y - h, z - l, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
        x    , y + h, z    , 0.0f, 0.0f, -1.0f, 0.5f, 1.0f,
        x + w, y - h, z - l, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,

        x - w, y - h, z + l, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        x    , y + h, z    , 0.0f, 0.0f, 1.0f, 0.5f, 1.0f,
        x + w, y - h, z + l, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,

        x - w, y - h, z - l, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        x    , y + h, z    , -1.0f, 0.0f, 0.0f, 0.5f, 1.0f,
        x - w, y - h, z + l, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

        x + w, y - h, z - l, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        x    , y + h, z    , 1.0f, 0.0f, 0.0f, 0.5f, 1.0f,
        x + w, y - h, z + l, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

        x - w, y - h, z - l, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        x + w, y - h, z - l, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        x - w, y - h, z + l, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,

        x - w, y - h, z + l, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        x + w, y - h, z + l, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
        x + w, y - h, z - l, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
    };

    GLMesh mesh;
    if (!createTexture(filename, mesh.textureId))
    {
        cout << "Failed to load texture " << filename << endl;
    }

    const GLuint floatsPerVertex = 3;
    const GLuint floatsPerNormal = 3;
    const GLuint floatsPerUV = 2;

    mesh.nVertices = sizeof(verts) / (sizeof(verts[0]) * (floatsPerVertex + floatsPerNormal + floatsPerUV));

    glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
    glBindVertexArray(mesh.vao);

    // Create 2 buffers: first one for the vertex data; second one for the indices
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo); // Activates the buffer
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

    // Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
    GLint stride = sizeof(float) * (floatsPerVertex + floatsPerNormal + floatsPerUV);// The number of floats before each

    // Create Vertex Attribute Pointers
    glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, floatsPerNormal, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * floatsPerVertex));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, floatsPerUV, GL_FLOAT, GL_FALSE, stride, (void*)(sizeof(float) * (floatsPerVertex + floatsPerNormal)));
    glEnableVertexAttribArray(2);

    gMeshVector.push_back(mesh);
}

// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint &programId)
{
    // Compilation and linkage error reporting
    int success = 0;
    char infoLog[512];

    // Create a Shader program object.
    programId = glCreateProgram();

    // Create the vertex and fragment shader objects
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

    // Retrive the shader source
    glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
    glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

    // Compile the vertex shader, and print compilation errors (if any)
    glCompileShader(vertexShaderId); // compile the vertex shader
    // check for shader compile errors
    glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glCompileShader(fragmentShaderId); // compile the fragment shader
    // check for shader compile errors
    glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

        return false;
    }

    // Attached compiled shaders to the shader program
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);

    glLinkProgram(programId);   // links the shader program
    // check for linking errors
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

        return false;
    }

    glUseProgram(programId);    // Uses the shader program

    return true;
}


// Destroys all the meshes
void UDestroyMesh()
{
    for (GLMesh& mesh : gMeshVector)
    {
        glDeleteVertexArrays(1, &mesh.vao);
        glDeleteBuffers(1, &mesh.vbo);
    }
}

// Destroys all the shader programs
void UDestroyShaderProgram()
{
    glDeleteProgram(gMeshProgramId);
}

// Destroys all the textures
void UDestroyTexture()
{
    for (GLMesh mesh : gMeshVector)
    {
        glGenTextures(1, &mesh.textureId);
    }
}

// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
    // GLFW: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLFW: window creation
    // ---------------------
    * window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
    if (*window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(*window);
    glfwSetFramebufferSizeCallback(*window, UResizeWindow);
    glfwSetCursorPosCallback(*window, UMousePositionCallback);
    glfwSetScrollCallback(*window, UMouseScrollCallback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLEW: initialize
    // ----------------
    // Note: if using GLEW version 1.13 or earlier
    glewExperimental = GL_TRUE;
    GLenum GlewInitResult = glewInit();

    if (GLEW_OK != GlewInitResult)
    {
        std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
        return false;
    }

    // Displays GPU OpenGL version
    cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

    return true;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{
    static const float cameraSpeed = 2.5f;

    // Camera Movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        gCamera.ProcessKeyboard(LEFT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        gCamera.ProcessKeyboard(RIGHT, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        gCamera.ProcessKeyboard(DOWN, gDeltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        gCamera.ProcessKeyboard(UP, gDeltaTime);

    // Perspective
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        if (!gIsPPressed)
        {
            gIsPPressed = true;
            gCamera.IsPerspective ^= 1; // Flip the boolean with bitwise
        }
    }
    else
        gIsPPressed = false;

    // Quit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the mouse moves, this callback is called
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (gFirstMouse)
    {
        gLastX = xpos;
        gLastY = ypos;
        gFirstMouse = false;
    }

    float xoffset = xpos - gLastX;
    float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

    gLastX = xpos;
    gLastY = ypos;

    gCamera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    gCamera.ProcessMouseScroll(yoffset);
}

// Functioned called to render a frame
void URender()
{
    // Enable z-depth
    glEnable(GL_DEPTH_TEST);

    // Clear the frame and z buffers
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Creates a projection based off perspective
    glm::mat4 projection;
    if (gCamera.IsPerspective)
        projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
    else
        projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, -100.0f, 100.0f);

    // Set the shader to be used
    glUseProgram(gMeshProgramId);

    // Retrieves and passes transform matrices to the Shader program
    GLint modelLoc = glGetUniformLocation(gMeshProgramId, "model");
    GLint viewLoc = glGetUniformLocation(gMeshProgramId, "view");
    GLint projLoc = glGetUniformLocation(gMeshProgramId, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::scale(glm::vec3(1.0f, 1.0f, 1.0f))));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(gCamera.GetViewMatrix()));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    // Reference matrix uniforms from the Mesh Shader program for the mesh's color, light color, light position, and camera position
    GLint objectColorLoc = glGetUniformLocation(gMeshProgramId, "objectColor");
    GLint lightColorLoc = glGetUniformLocation(gMeshProgramId, "lightColor");
    GLint lightPositionLoc = glGetUniformLocation(gMeshProgramId, "lightPos");
    GLint viewPositionLoc = glGetUniformLocation(gMeshProgramId, "viewPosition");

    // Pass color, light, and camera data to the Mesh Shader program's corresponding uniforms
    glUniform3f(objectColorLoc, gObjectColor.r, gObjectColor.g, gObjectColor.b);
    glUniform3f(lightColorLoc, gLightColor.r, gLightColor.g, gLightColor.b);
    glUniform3f(lightPositionLoc, gLightPosition.x, gLightPosition.y, gLightPosition.z);
    const glm::vec3 cameraPosition = gCamera.Position;
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

    // Activate UV
    GLint UVScaleLoc = glGetUniformLocation(gMeshProgramId, "uvScale");
    glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));

    // Activate VBOs within each mesh
    for (GLMesh &mesh : gMeshVector)
    {
        glBindVertexArray(mesh.vao);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mesh.textureId);
        glDrawArrays(GL_TRIANGLES, 0, mesh.nVertices);
    }

    // Deactivate the Vertex Array Object
    glBindVertexArray(0);
    glUseProgram(0);

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
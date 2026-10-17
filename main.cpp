#include <GL/glew.h>

#include "headers/shader.h"
#include "headers/texture.h"
#include "headers/mesh.h"
#include "headers/window.h"

#include <iostream>
#include <unordered_map>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Material {
    glm::vec3 colour;
    float reflectivity;
    float specularFactor;
};

const int POLL_DELAY = 33; // In milliseconds. Set to 30FPS by default, decrease POLL_DELAY to 17ms if you want ~60FPS (but be extra careful to keep things optimized)
const int WORKGROUP_DIM = 16; // Must match compute shader x and y dimension
float globalTime = 0.0f; // In seconds
float cubeAngle = 0.0f;
const float ROT_SCALE = 0.05f;

std::unordered_map<std::string, Texture> textures;
std::vector<Material> materials;
GLuint frameTextureId = 0, materialSSBO;
glm::vec3 lightPosition;
ShaderProgram raymarchCompProgram, displayProgram;
Mesh rectangle;
float rayPlaneDepth = 1.0f, rayPlaneHalfWidth, rayPlaneHalfHeight; // Ray creation parameters. Calculated using screen fov and aspect ratio in the reshape() function
bool showMarchCount = false;
bool showAO = false;
CubemapTexture skyTexture;

void raymarchToTexture(ShaderProgram& computeProgram, GLuint texId) {
    glm::vec3 rayFwd = window::fwd * rayPlaneDepth;
    glm::vec3 rayRgt = window::rgt * rayPlaneHalfWidth;
    glm::vec3 rayUp  = window::up * rayPlaneHalfHeight;
	glUniform3fv(computeProgram.getUniformLocation("rayFwd"), 1, glm::value_ptr(rayFwd)); 
	glUniform3fv(computeProgram.getUniformLocation("rayRgt"), 1, glm::value_ptr(rayRgt)); 
	glUniform3fv(computeProgram.getUniformLocation("rayUp"),  1, glm::value_ptr(rayUp)); 
	glUniform3fv(computeProgram.getUniformLocation("eyePos"), 1, glm::value_ptr(window::eyePos)); 
	glUniform3fv(computeProgram.getUniformLocation("lightPos"), 1, glm::value_ptr(lightPosition)); 

    glUniform1f(computeProgram.getUniformLocation("globalTime"), globalTime);
    glUniform1f(computeProgram.getUniformLocation("cubeAngle"), cubeAngle);
    // Bind material SSBO to index 0 (binding = 0 in shader)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, materialSSBO);

    // Bind the frame texture as an image2D at location=0
    glBindImageTexture(0, texId, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    glDispatchCompute(((window::width-1)/WORKGROUP_DIM)+1, ((window::height-1)/WORKGROUP_DIM)+1, 1);
    // This instruction makes the GPU wait until previously queued commands have finished writing to the image (so that the frame texture updates complete before the next GPU call)
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT); 
}

void drawTexture(ShaderProgram& program, GLuint texId, Mesh& screenRect) {
    // Bind the texture to unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);
    
    glBindVertexArray(screenRect.vaoID);
    glDrawArrays(screenRect.primitiveType, 0, screenRect.verts.size());
}

GLuint createFrameTexture(int width, int height) {
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    // image2Ds are supposed to use immutable storage (which glTexStorage2D creates) rather than mutable storage (e.g. glTexImage2D)
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, width, height);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return texId;
}

// Setup GL state, load and compile shaders, load required data
void init() {
    // Initialize OpenGL state. Since we are just rendering a single rectangle, no need to enable backface culling or depth testing
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // The viewer starts in an elevated position, looking slightly downwards
    window::eyePos = glm::vec3(0.0f, 3.0f, 10.0f);
    window::angleVert = 0.3f;
    window::moveSpeed = 0.005;
	lightPosition = glm::vec3(10.0f, 20.0f, 40.0f);

    // Load the raymarching program (compute shader)
    raymarchCompProgram = ShaderProgram({
        {"../shaders/raymarch.comp", GL_COMPUTE_SHADER}
    });
    raymarchCompProgram.setAsActiveProgram();
    glUniform1i(raymarchCompProgram.getUniformLocation("showMarchCount"), showMarchCount); // Set program not to visualize march count by default

    skyTexture = CubemapTexture({
        "../assets/autumn_field_puresky/px.png",
        "../assets/autumn_field_puresky/nx.png",
        "../assets/autumn_field_puresky/ny.png",
        "../assets/autumn_field_puresky/py.png",
        "../assets/autumn_field_puresky/pz.png",
        "../assets/autumn_field_puresky/nz.png",
    });

    glUniform1i(raymarchCompProgram.getUniformLocation("skyTexture"), 1);
    glActiveTexture(GL_TEXTURE1);
    skyTexture.bind();
    glActiveTexture(GL_TEXTURE0);




    // Initialize material data
    materials = {
        {glm::vec3(0.6f, 0.0f, 0.0f), 0.3f, 1.0f}, // Material index 0.  colour, reflective, has strong specular highlight
        {glm::vec3(0.0, 0.600f, 1.0f), 0.4f, 0.5f}, // Material index 1. colour, not reflective, weak specular highlight
        {glm::vec3(1, 0.643f, 0.0f), 0.0f, 0.0f}, // boxframe
        {glm::vec3(0.0, 0.800f, 0.094f), 0.6f, 1.0f}, // octahedron
    };
    
    // Create materials SSBO and load material data into it
    glGenBuffers(1, &materialSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, materialSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, materials.size()*sizeof(Material), materials.data(), GL_STATIC_COPY);

    // Initialize rectangle that covers the screen in clip space
    rectangle = Mesh(GL_TRIANGLE_STRIP);
    rectangle.addVert(glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec2(0.0f, 0.0f));
    rectangle.addVert(glm::vec3( 1.0f, -1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec2(1.0f, 0.0f));
    rectangle.addVert(glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec2(0.0f, 1.0f));
    rectangle.addVert(glm::vec3( 1.0f,  1.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f), glm::vec2(1.0f, 1.0f));
    rectangle.updateBuffers();

    // Load the program to display the texture-mapped rectangle
    displayProgram = ShaderProgram({
		{"../shaders/display_texture.vert", GL_VERTEX_SHADER}, 
		{"../shaders/display_texture.frag", GL_FRAGMENT_SHADER}
	});
    displayProgram.setAsActiveProgram();
	glUniform1i(displayProgram.getUniformLocation("frameTexture"), 0); // Frame colour texture sampler will use texturing unit 0
}

// Frame update function - run at ~30fps (based on POLL_DELAY), update necessary uniforms and queue a new frame to render
void update(int value) {
	glutTimerFunc(POLL_DELAY, update, 0);
	glutPostRedisplay();

	if (!window::paused)  {
        globalTime += 0.001f * POLL_DELAY;
        cubeAngle += ROT_SCALE * POLL_DELAY;
    }
	window::updateUserPosition(POLL_DELAY); // Updates user position and view transform based on keyboard/mouse inputs
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Dispatch the compute shader to draw ray marched scene to a texture
    raymarchCompProgram.setAsActiveProgram();
    raymarchToTexture(raymarchCompProgram, frameTextureId);

    // Draw a rectangle over the screen, which is textured with our ray marched texture
    displayProgram.setAsActiveProgram();
    drawTexture(displayProgram, frameTextureId, rectangle);

    glutSwapBuffers();
}

// Wrapper over window::keyboard that adds some custom keybinds
void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        // Overwrite wireframe keybind to instead show number of marches for each pixel
        case 'z': case 'Z': 
            showMarchCount = !showMarchCount;
            raymarchCompProgram.setAsActiveProgram();
            glUniform1i(raymarchCompProgram.getUniformLocation("showMarchCount"), showMarchCount);
            break;
        case 'l': case 'L':
            showAO = !showAO;
            raymarchCompProgram.setAsActiveProgram();
            glUniform1i(raymarchCompProgram.getUniformLocation("showAO"), showAO);
            break;
        default: window::keyboard(key, x, y);
    }
}

// Wrapper over window::reshape that also resizes the frame texture
void reshape(int w, int h) {
    window::reshape(w, h);
    // Because the frame texture uses immutable storage, it must be deleted and created anew. It cannot be resized.
    if (frameTextureId != 0) glDeleteTextures(1, &frameTextureId);
    frameTextureId = createFrameTexture(w, h);

    raymarchCompProgram.setAsActiveProgram();
	glUniform1i(raymarchCompProgram.getUniformLocation("screenWidth"), window::width);
	glUniform1i(raymarchCompProgram.getUniformLocation("screenHeight"), window::height);

    float aspectRatio = float(window::width)/window::height;

    rayPlaneHalfHeight = rayPlaneDepth * tan(window::fov/2.0f);
    rayPlaneHalfWidth = aspectRatio * rayPlaneHalfHeight;
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitContextVersion(4, 3);
    glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(window::width, window::height);
    glutCreateWindow("COSC422: Simple Raymarching");

    if (glewInit() != GLEW_OK) {
        std::cerr << "Unable to initialize GLEW  ...exiting." << std::endl;
        exit(EXIT_FAILURE);
    }

    window::init(); // Register user input callbacks
    init(); // Load shaders, textures, meshes
    glutKeyboardFunc(keyboard);
    glutReshapeFunc(reshape);

    glutDisplayFunc(display);
    glutTimerFunc(POLL_DELAY, update, 0); 
    glutMainLoop();
}

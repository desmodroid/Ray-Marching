/* COSC422 Advanced Computer Graphics (2026)

mesh.h: Provides a Mesh class that encapsulates the data and sets up the necessary VAO/VBO/EBO needed for rendering a single mesh.
Each vertex is defined using the Vertex struct, which contains position, normal, colour, texture coordinate, bone ID, and bone weight attributes. For
much of COSC422, we will not worry about the final two attributes (boneIDs and boneWeights) - these will only become relevant once we reach the skeletal
animation topic.
Each mesh uses only a single VBO, which contains all six vertex attributes interleaved together. The VAO is set up with the appropriate pointers to access
each attribute separately in the vertex shader.

IMPORTANT: After any changes are made to the mesh.verts or mesh.elements arrays, you MUST call mesh.updateBuffers() before attempting to draw the mesh.
Your application will probably crash otherwise when the shaders attempt to access memory that hasn't been buffered properly!

Notable public attributes that may be useful to edit for a Mesh object:
 - mesh.verts: this is a std::vector of vertices that you can either edit directly, or by using mesh.addVert()
 - mesh.elements: this is a std::vector of element indices that you can edit directly, or by using mesh.addElements()
 - mesh.specColour: the specular colour of the mesh, which defaults to vec3(1.0). Set it to vec3(0.0) to turn off specular reflections for the mesh
 - mesh.modelTransform: the model transform of the mesh (transforms from model space to world space). Optional, defaults to the identity matrix
 - mesh.diffuseTexture: the Texture (texture.h) that should be for diffuse (and ambient if using Phong-Blinn) colouring of the mesh. Optional
 - mesh.specTexture: the Texture (texture.h) that should be for specular colouring of the mesh. Optional
*/

#ifndef MESH_H
#define MESH_H

#include <vector>
#include <unordered_map>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include "texture.h"

// Vertex struct for packing into a VBO
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 colour;
    glm::vec2 texCoord;

    // Used only for rigged meshes. IDs and weights for up to 4 bones on each vertex
    glm::ivec4 boneIDs;
    glm::vec4 boneWeights;
};

class Mesh {
    private:
        GLuint vboID, eboID;
    public:
        GLuint vaoID, primitiveType, hint = GL_STATIC_DRAW;
        std::vector<Vertex> verts;
        std::vector<unsigned int> elements;
        glm::vec3 specColour = glm::vec3(1.0f);
        glm::mat4 modelTransform = glm::mat4(1.0f);
        Texture diffuseTexture, specTexture;
        
        // Used only for rigged meshes:
        std::unordered_map<std::string, unsigned int> boneIndices; // A mapping of Assimp scene graph node names -> bone indices in this mesh
        std::unordered_map<unsigned int, glm::mat4> boneOffsetMatrices; // A mapping of bone index -> bone offset matrix

        Mesh() {}
        Mesh(GLuint primitiveType) {
            this->primitiveType = primitiveType;
            glGenVertexArrays(1, &vaoID);
            glGenBuffers(1, &vboID); // Buffer for vertex data
            glGenBuffers(1, &eboID); // Buffer for element data (face indices)

            // Bind the VAO for this mesh
            glBindVertexArray(vaoID);

            // Positions
            glBindBuffer(GL_ARRAY_BUFFER, vboID); 
            glEnableVertexAttribArray(0); //enable location 0 to be accessed as a per-vertex attribute within the vertex shader
            // Set vertex location 0 to point to VBO 0 (currently bound), 3 floats per vertex, step size 76 bytes (sizeof Vertex), offset 0*4 bytes
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position)); 

            // Normals
            glEnableVertexAttribArray(1);
            // Set vertex location 1 to point to VBO 0 (currently bound), 3 floats per vertex, step size 76 bytes (sizeof Vertex), offset 3*4 bytes
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

            // Colours
            glEnableVertexAttribArray(2);
            // Set vertex location 2 to point to VBO 0 (currently bound), 3 floats per vertex, step size 76 bytes (sizeof Vertex), offset 6*4 bytes
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, colour));

            // Tex coords
            glEnableVertexAttribArray(3);
            // Set vertex location 3 to point to VBO 0 (currently bound), 2 floats per vertex, step size 76 bytes (sizeof Vertex), offset 9*4 bytes
            glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));


            // Used only for rigged meshes:
            glEnableVertexAttribArray(4);
            glVertexAttribIPointer(4, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, boneIDs));
            glEnableVertexAttribArray(5);
            glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, boneWeights));
        }

        // Update the GPU-accessible buffers for this mesh (the VBO and EBO)
        void updateBuffers() {
            glBindVertexArray(vaoID);
            glBindBuffer(GL_ARRAY_BUFFER, vboID);
            glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(Vertex), verts.data(), hint);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size()*sizeof(unsigned int), elements.data(), hint);
        }

        void addVert(glm::vec3 position, glm::vec3 normal, glm::vec3 colour=glm::vec3(1.0f), glm::vec2 texCoord=glm::vec2(0.0f)) {
            Vertex v;
            v.position = position;
            v.normal = normal;
            v.colour = colour;
            v.texCoord = texCoord;
            v.boneIDs = glm::ivec4(-1);
            verts.push_back(v);
        }

        void addElements(std::vector<unsigned int> newElements) {
            elements.insert(elements.end(), newElements.begin(), newElements.end());
        }
};

#endif // MESH_H
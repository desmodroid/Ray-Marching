/* COSC422 Advanced Computer Graphics (2026)

window.h: Defines freeglut callbacks to control user inputs (keyboard, mouse, window reshape), and calculate the appropriate view and projection matrices.

Controls are as follows:
WASD - movement forward/left/backward/right
QE - movement up and down
Z - toggle wireframe mode
space - toggle pause/unpause time
left click + drag - rotate camera
*/

#ifndef WINDOW_H
#define WINDOW_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/freeglut.h>

namespace window {
    // Keyboard movement and mouse drag speeds. Can be adjusted as needed
    float moveSpeed = 0.01f;
    float dragSpeed = 1.6f;

    // Initial window dimensions
    int width = 1024, height = 1024;

    // Field of view - used in perspective transform
    float fov = glm::radians(70.0f);

    // Viewer's position and look angles
    glm::vec3 eyePos = glm::vec3(0.0f);
    float angleHoriz = 0.0f, angleVert = 0.0f;

    // View and projection matrices used for transforming vertices from world-space into view (or eye)-space and then clip-space
    glm::mat4 proj, view;
    
    glm::vec3 fwd, rgt, up;	// Axes of the view coordinate space
    int fwdAxisDelta = 0, rgtAxisDelta = 0, upAxisDelta = 0; // User input for movement up or down each axis
    bool mouseIsDown = false, paused = false, wireframe = false;
    int mouseX, mouseY; // Used for calculating motion when dragging the mouse

    void updateViewTransform() {
        // Find a rotation matrix to represent the viewer's rotation (combining a horizontal and vertical rotation)
        glm::mat4 viewRotationMat = glm::rotate(glm::mat4(1.0f), angleHoriz, glm::vec3(0.0f, 1.0f, 0.0f));
        viewRotationMat = glm::rotate(viewRotationMat, angleVert, glm::vec3(-1.0f, 0.0f, 0.0f));

        // Apply the rotation matrix to the view and right vectors to update these
        fwd = glm::vec3(viewRotationMat * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
        rgt = glm::vec3(viewRotationMat * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
        up  = glm::vec3(viewRotationMat * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));
        // Update the view matrix. the look position is found as eyePos + view vector ('fwd')
        view = glm::lookAt(eyePos, eyePos+fwd, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void updateProjTransform() {
        proj = glm::perspective(fov, float(width)/height, 0.1f, 10000.0f);
    }

    // Move the user along the view axes depending on the time elapsed and buttons currently pressed
    void updateUserPosition(int deltaTime) {
        // Moving forward or backward (based on look direction 'fwd')
        if (fwdAxisDelta != 0) eyePos += glm::vec3(moveSpeed*deltaTime*fwdAxisDelta)*fwd;
        // Moving left or right (based on right of look direction 'rgt')
        if (rgtAxisDelta != 0) eyePos += glm::vec3(moveSpeed*deltaTime*rgtAxisDelta)*rgt;
        // Moving up or down (locked to up and down regardless of look dir)
        if (upAxisDelta  != 0) eyePos += glm::vec3(0, moveSpeed*deltaTime*upAxisDelta, 0);
        
        // Update view transform matrix
        updateViewTransform();
    }

    // Handle key press events
    void keyboard(unsigned char key, int x, int y) {
        switch(key) {
            case 'w': case 'W': fwdAxisDelta = 1; break;
            case 's': case 'S': fwdAxisDelta = -1; break;
            case 'a': case 'A': rgtAxisDelta = -1; break;
            case 'd': case 'D': rgtAxisDelta = 1; break;
            case 'e': case 'E': upAxisDelta = 1; break;
            case 'q': case 'Q': upAxisDelta = -1; break;
            case 'z': case 'Z': 
                wireframe = !wireframe;
                glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
                break;
        };
    }

    // Handle key up events
    void keyboardUp(unsigned char key, int x, int y) {
        switch (key) {
            case 'w': case 'W': fwdAxisDelta = fwdAxisDelta > 0 ? 0 : fwdAxisDelta; break;
            case 's': case 'S': fwdAxisDelta = fwdAxisDelta < 0 ? 0 : fwdAxisDelta; break;
            case 'a': case 'A': rgtAxisDelta = rgtAxisDelta < 0 ? 0 : rgtAxisDelta; break;
            case 'd': case 'D': rgtAxisDelta = rgtAxisDelta > 0 ? 0 : rgtAxisDelta; break;
            case 'e': case 'E': upAxisDelta  = upAxisDelta  > 0 ? 0 : upAxisDelta; break;
            case 'q': case 'Q': upAxisDelta  = upAxisDelta  < 0 ? 0 : upAxisDelta; break;
            case ' ': paused = !paused; break;
        }
    }

    // Handle mouse button down (click) and up (unclick) events
    void mouse(int button, int state, int x, int y) {
        if (button == GLUT_LEFT_BUTTON) {
            mouseIsDown = state == GLUT_DOWN;
            mouseX = x;
            mouseY = y;
        }
    }

    // Handle mouse movement events
    void motion(int x, int y) {
        if (mouseIsDown) {
            angleHoriz -= (float)(x - mouseX)*dragSpeed/height;
            angleVert  += (float)(y - mouseY)*dragSpeed/height;
            // Clamp vertical angle to stay between -90 and +90 degrees
            angleVert = glm::clamp(angleVert, -glm::radians(89.9f), glm::radians(89.9f));
            mouseX = x;
            mouseY = y;
        }
    }
    
    // Update viewport and perspective transform based on new width and height
    void reshape(int w, int h) {
        width = w;
        height = h;
        glViewport(0, 0, width, height);
        updateProjTransform();
    }

    // Register user input callbacks, initialize the view and projection matrices
    void init() {
        glutKeyboardFunc(window::keyboard);
        glutKeyboardUpFunc(window::keyboardUp);
        glutMouseFunc(window::mouse);
        glutMotionFunc(window::motion);
        glutReshapeFunc(window::reshape);
        
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        
        updateProjTransform();
        updateViewTransform();
    }
}

#endif // WINDOW_H
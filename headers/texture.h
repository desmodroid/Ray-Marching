/* COSC422 Advanced Computer Graphics (2026)

texture.h: Provides a Texture class that handles loading of images into OpenGL textures (using stb_image.h).
Also provides a CubemapTexture class that handles loading of six images into an RGB cubemap.
*/

#ifndef TEXTURE_H
#define TEXTURE_H

#include <iostream>
#include <filesystem>
#include <GL/freeglut.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Modify a file path to use the correct forward or back slash depending on the OS
std::string fixPathSlashes(std::string filePath) {
    for (char& c : filePath) if (c == '/' || c == '\\') c = std::filesystem::path::preferred_separator;
    return filePath;
}

class Texture {
    public:
        int width, height, channels = -1; // Invalid or uninitialized texture will have -1 channels
        
        Texture() {}
        // Load from file
        Texture(std::string filePath, GLuint wrapMode, bool flip_y=true, bool verbose=true) {
            filePath = fixPathSlashes(filePath);
            int ok = stbi_info(filePath.c_str(), &width, &height, &channels);
            setFormat();
            if (ok != 1 || channels == -1) {
                std::cerr << "Failed to load " << filePath << std::endl;
                return;
            }

            // OpenGL textures expect the y dimension to be oriented opposite to how it is defined in most images, so we will flip y by default
            stbi_set_flip_vertically_on_load(flip_y);
            unsigned char* imageData = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
            if (verbose) std::cout << "Loaded texture " << filePath << " (" << channels << " channels, " << width << "x" << height << "px)" << std::endl;
            setupBuffers(imageData, wrapMode);

            stbi_image_free(imageData);
        }
        // Load from vector
        Texture(std::vector<unsigned char>& imageData, GLuint wrapMode, int width, int height, int channels) {
            this->width = width;
            this->height = height;
            this->channels = channels;
            setFormat();
            setupBuffers(&imageData[0], wrapMode);
        }

        void bind() {
            glBindTexture(GL_TEXTURE_2D, texId);
        }

        bool isValid() {
            return channels != -1;
        }
    private:
        GLuint texId, format;

        void setFormat() {
            switch (channels) {
                case 1: format = GL_RED; break;
                case 3: format = GL_RGB; break;
                case 4: format = GL_RGBA; break;
                default: channels = -1;
            }
        }

        void setupBuffers(unsigned char* imageData, GLuint wrapMode) {
            glGenTextures(1, &texId);
            bind();
            // Send the image data into an OpenGL buffer that can be accessed inside shaders
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, imageData);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
        }
};

class CubemapTexture {
    public:
        GLuint texId;
        int dim, channels = -1; // Invalid or uninitialized texture will have -1 channels
        
        CubemapTexture() {}
        // Load from vector of file paths
        CubemapTexture(std::vector<std::string> texturePaths, bool flip_y=true, bool verbose=true) {
            if (texturePaths.size() != 6) {
                std::cout << "Expected 6 texture paths, but got " << texturePaths.size() << ". Cubemap will be invalid." << std::endl;
                return;
            }
            // OpenGL textures expect the y dimension to be oriented opposite to how it is defined in most images, so we will flip y by default
            stbi_set_flip_vertically_on_load(flip_y);

            glGenTextures(1, &texId);
            glBindTexture(GL_TEXTURE_CUBE_MAP, texId);
            int currentWidth, currentHeight, currentChannels;
            for (int side = 0; side < 6; side++) {
                std::string filePath = fixPathSlashes(texturePaths[side]);
                // Force imageData to have 3 channels even if the input data came with more or less
                unsigned char* imageData = stbi_load(filePath.c_str(), &currentWidth, &currentHeight, &currentChannels, 3);
                // Catch any failed loads
                if (imageData == nullptr) {
                    std::cout << "Failed to load " << filePath << ", cubemap will be invalid." << std::endl;
                    return;
                }
                
                // Set resolution based on the first image loaded
                if (side == 0) dim = currentWidth;
                
                // Catch any images with invalid resolution. All images must be square and the same resolution as each other.
                if (currentWidth != dim || currentHeight != dim) {
                    std::cout << "Mismatching image dimensions: " << filePath << " is " << currentWidth << "x" << currentHeight << 
                                                            ", but expected " << dim << "x" << dim << ". Cubemap will be invalid." << std::endl;
                    stbi_image_free(imageData);
                    return;
                }
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, 0, GL_RGB, dim, dim, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);
                stbi_image_free(imageData);
            }
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            channels = 3;
            
            if (verbose) std::cout << "Loaded cubemap texture (3 channels, " << dim << "x" << dim << "px x6)" << std::endl;
        }

        void bind() {
            glBindTexture(GL_TEXTURE_CUBE_MAP, texId);
        }

        bool isValid() {
            return channels != -1;
        }
};

#endif // TEXTURE_H
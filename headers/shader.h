/* COSC422 Advanced Computer Graphics (2026)

shader.h: Provides a ShaderProgram class that handles loading and linking shaders into a OpenGL shader program. It also facilitates registering and fetching uniform locations.
*/

#ifndef SHADER_H
#define SHADER_H

#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>

class ShaderProgram {
    public:
        ShaderProgram() {}
        ShaderProgram(const std::unordered_map<std::string, GLuint>& shadersStages) {
            programID = glCreateProgram();
            for (auto it = shadersStages.begin(); it != shadersStages.end(); it++) {
                GLuint shaderID = loadShader(it->second, it->first);
                glAttachShader(programID, shaderID);
            }
            glLinkProgram(programID);
            checkErrors(programID, false);
            setAsActiveProgram();
        }

        void setAsActiveProgram() {
            glUseProgram(programID);
        }

        // Use this function to get uniform locations for this shader program. It will only perform the slow glGetUniformLocation() call the first time a uniform location 
        // is accessed. Subsequent calls will instead fetch the uniform location from a hashmap (std::unordered_map).
        GLuint getUniformLocation(const std::string& name) {
            if (uniforms.count(name) == 0) registerUniform(name);
            return uniforms[name];
        }
    
    private:
        GLuint programID;
        // Uniform locations for variables
        std::unordered_map<std::string, GLuint> uniforms;

        // Check for compilation errors during shader compilation or program linking
        void checkErrors(GLuint id, bool isShader) {
            GLint status;
            if (isShader) glGetShaderiv(id, GL_COMPILE_STATUS, &status);
                    else  glGetProgramiv(id, GL_LINK_STATUS, &status);

            if (status == GL_FALSE)	{ // There was some kind of error
                GLint infoLogLength;
                if (isShader) glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
                        else  glGetProgramiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);

                GLchar *strInfoLog = new GLchar[infoLogLength + 1];
                if (isShader) {
                    glGetShaderInfoLog(id, infoLogLength, NULL, strInfoLog);
                    std::cerr << "Compile failure in shader: " << strInfoLog << std::endl;
                } else {
                    glGetProgramInfoLog(id, infoLogLength, NULL, strInfoLog);
                    std::cerr << "Failed to link shader program: " << strInfoLog << std::endl;
                }
                delete[] strInfoLog;
            }
        }

        // Load and compile a shader from a file
        GLuint loadShader(GLenum shaderType, std::string filename) {
            std::ifstream shaderFile(filename.c_str());
            if(!shaderFile.good()) std::cout << "Error opening shader file." << std::endl;
            std::stringstream shaderData;
            shaderData << shaderFile.rdbuf();
            shaderFile.close();
            std::string shaderStr = shaderData.str();
            const char* shaderTxt = shaderStr.c_str();

            GLuint shader = glCreateShader(shaderType);
            glShaderSource(shader, 1, &shaderTxt, NULL);
            glCompileShader(shader);
            checkErrors(shader, true);
            return shader;
        }

        void registerUniform(const std::string& name) {
            uniforms[name] = glGetUniformLocation(programID, name.c_str());
        }

};

#endif // SHADER_H